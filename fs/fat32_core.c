#include "fat32_core.h"
#include "../kernel/ata_pio.h"
#include "../kernel/util.h"
#include "../kernel/tty.h"

static int stricmp(const char* a, const char* b){
    while (*a && *b){
        char ca = *a, cb = *b;
        if (ca>='a'&&ca<='z') ca-=32;
        if (cb>='a'&&cb<='z') cb-=32;
        if (ca!=cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
        ++a; ++b;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}
static void upper_copy(char* d, const char* s, int n){
    int i=0; for(; s[i] && i<n-1; ++i){ char c=s[i]; if(c>='a'&&c<='z') c-=32; d[i]=c; } d[i]=0;
}
static uint32_t rd32(const uint8_t* p){ return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
static uint16_t rd16(const uint8_t* p){ return (uint16_t)p[0] | ((uint16_t)p[1]<<8); }

/* читать count секторов по одному */
static int read_sectors(uint8_t drive, uint32_t lba, uint32_t count, void* out){
    (void)drive; /* у нас один master на 0x1F0 */
    uint8_t* dst = (uint8_t*)out;
    for (uint32_t i=0;i<count;i++){
        if (ata_read28(lba+i, 1, dst + 512*i) != 0) return -1;
    }
    return 0;
}

static uint32_t cluster_first_lba(fat32_t* fs, uint32_t cl){
    return fs->first_data_lba + (cl - 2) * fs->spc;
}
static int is_eoc(uint32_t v){ return (v >= 0x0FFFFFF8); }

static uint32_t fat_get_next(fat32_t* fs, uint32_t cl){
    uint32_t fat_offset = cl * 4;
    uint32_t fat_sector = fs->first_fat_lba + (fat_offset / 512);
    uint32_t ent_off    = fat_offset % 512;
    uint8_t sec[512];
    if (read_sectors(fs->drive, fat_sector, 1, sec) != 0) return 0x0FFFFFFF;
    uint32_t val = rd32(&sec[ent_off]) & 0x0FFFFFFF;
    return val;
}

int fat_mount(uint8_t drive, fat32_t* out){
    if (!out) return -1;
    kmemset(out, 0, sizeof(*out));

    uint8_t mbr[512];
    if (read_sectors(drive, 0, 1, mbr) != 0){ vga_puts("Read MBR fail\n"); return -1; }
    if (mbr[510]!=0x55 || mbr[511]!=0xAA){ vga_puts("Bad MBR sig\n"); return -1; }

    const uint8_t* p = &mbr[0x1BE];
    uint32_t part_lba = 0;
    for (int i=0;i<4;i++, p+=16){
        uint8_t  type = p[4];
        uint32_t lba  = rd32(&p[8]);
        if (type==0x0B || type==0x0C){ part_lba = lba; break; }
    }
    if (!part_lba){ vga_puts("FAT32 part not found\n"); return -1; }

    uint8_t bpb[512];
    if (read_sectors(drive, part_lba, 1, bpb)!=0){ vga_puts("Read BPB fail\n"); return -1; }

    uint16_t Byps = rd16(&bpb[11]);
    uint8_t  Spc  = bpb[13];
    uint16_t Rsvd = rd16(&bpb[14]);
    uint8_t  Nf   = bpb[16];
    uint32_t Fsz  = rd32(&bpb[36]);
    uint32_t Rcl  = rd32(&bpb[44]);

    if (Byps!=512 || Spc==0 || Nf==0 || Fsz==0){ vga_puts("BPB bad\n"); return -1; }

    out->drive = drive;
    out->valid = 1;
    out->part_lba_begin = part_lba;
    out->byps = Byps; out->spc = Spc; out->rsvd = Rsvd; out->nfats=Nf; out->fatsz=Fsz; out->rootclus=Rcl;
    out->first_fat_lba  = part_lba + Rsvd;
    out->first_data_lba = part_lba + Rsvd + (uint32_t)Nf * Fsz;

    vga_puts("FAT32 OK. root="); vga_puthex(out->rootclus); vga_puts("\n");
    return 0;
}

typedef struct __attribute__((packed)){
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt;
    uint8_t crt_tenths;
    uint16_t crt_time, crt_date, lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time, wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} dirent_t;

static int match_83(const uint8_t n11[11], const char* path83){
    char name[13]; int k=0;
    for(int j=0;j<8 && n11[j]!=' '; ++j) name[k++]=n11[j];
    int has_ext = (n11[8]!=' ');
    if (has_ext){ name[k++]='.'; for(int j=8;j<11 && n11[j]!=' '; ++j) name[k++]=n11[j]; }
    name[k]=0;
    return stricmp(name, path83)==0;
}

static void upper_copy(char* d, const char* s, int n);
static int normalize_name(const char* path, char* out, int outsz){
    while (*path==' ' || *path=='/') ++path;
    if (!*path) return -1;
    char up[64]; upper_copy(up, path, sizeof(up));
    k_strlcpy(out, up, outsz);
    return 0;
}

static int find_in_dir_root(fat32_t* fs, const char* name83, uint32_t* out_clus, uint32_t* out_size, int* out_isdir){
    uint32_t cl = fs->rootclus;
    uint8_t sec[512];

    for(;;){
        uint32_t lba = cluster_first_lba(fs, cl);
        for (uint8_t s=0; s<fs->spc; ++s){
            if (read_sectors(fs->drive, lba+s, 1, sec)!=0) return -1;
            for (int off=0; off<512; off+=32){
                const dirent_t* de = (const dirent_t*)&sec[off];
                if (de->name[0]==0x00) return -1;
                if ((uint8_t)de->name[0]==0xE5) continue;
                if (de->attr==0x0F) continue;
                if (match_83(de->name, name83)){
                    uint32_t hi = de->fst_clus_hi, lo = de->fst_clus_lo;
                    *out_clus = ((hi<<16)|lo);
                    *out_size = de->file_size;
                    *out_isdir = (de->attr & 0x10) ? 1:0;
                    return 0;
                }
            }
        }
        uint32_t nxt = fat_get_next(fs, cl);
        if (is_eoc(nxt)) break;
        cl = nxt;
    }
    return -1;
}

void fat_ls_root(fat32_t* fs){
    if (!fs || !fs->valid) return;
    uint32_t cl = fs->rootclus;
    uint8_t sec[512];
    char out[14];

    for(;;){
        uint32_t lba = cluster_first_lba(fs, cl);
        for (uint8_t s=0; s<fs->spc; ++s){
            if (read_sectors(fs->drive, lba+s, 1, sec)!=0) return;
            for (int off=0; off<512; off+=32){
                const dirent_t* de = (const dirent_t*)&sec[off];
                if (de->name[0]==0x00) return;
                if ((uint8_t)de->name[0]==0xE5) continue;
                if (de->attr==0x0F) continue;

                int k=0;
                for (int j=0;j<8 && de->name[j]!=' '; ++j) out[k++]=de->name[j];
                if (de->name[8]!=' '){ out[k++]='.'; for(int j=8;j<11 && de->name[j]!=' '; ++j) out[k++]=de->name[j]; }
                out[k]=0;
                vga_puts(out); vga_puts("\n");
            }
        }
        uint32_t nxt = fat_get_next(fs, cl);
        if (is_eoc(nxt)) break;
        cl = nxt;
    }
}

int fat_read_file(fat32_t* fs, const char* path, void* buf, uint32_t buf_sz, uint32_t* out_read){
    if (out_read) *out_read=0;
    if (!fs || !fs->valid || !path || !buf || buf_sz==0) return -1;

    char name83[64];
    if (normalize_name(path, name83, sizeof(name83))!=0) return -1;

    uint32_t fclus=0, fsize=0; int isdir=0;
    if (find_in_dir_root(fs, name83, &fclus, &fsize, &isdir)!=0 || isdir) return -1;

    uint8_t sec[512];
    uint32_t remain = fsize;
    uint8_t* dst = (uint8_t*)buf;

    uint32_t cl = fclus;
    while (remain>0){
        uint32_t lba = cluster_first_lba(fs, cl);
        for (uint8_t s=0; s<fs->spc && remain>0; ++s){
            if (read_sectors(fs->drive, lba+s, 1, sec)!=0) return -1;
            uint32_t chunk = (remain>512)?512:remain;
            kmemcpy(dst, sec, chunk);
            dst += chunk; remain -= chunk;
        }
        uint32_t nxt = fat_get_next(fs, cl);
        if (is_eoc(nxt)) break;
        cl = nxt;
    }
    if (out_read) *out_read = fsize - remain;
    return 0;
}
