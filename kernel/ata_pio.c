#include <stdint.h>
#include "tty.h"

static inline void outb(uint16_t p, uint8_t v){ __asm__ __volatile__("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ __volatile__("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void io_delay(){ for(volatile int i=0;i<1000;i++){} }

#define ATA_IO    0x1F0
#define ATA_CTRL  0x3F6

#define R_DATA    (ATA_IO+0)
#define R_ERROR   (ATA_IO+1)
#define R_SECTCNT (ATA_IO+2)
#define R_LBA0    (ATA_IO+3)
#define R_LBA1    (ATA_IO+4)
#define R_LBA2    (ATA_IO+5)
#define R_HDDEVSEL (ATA_IO+6)
#define R_CMD     (ATA_IO+7)
#define R_STATUS  (ATA_IO+7)

#define SR_BSY 0x80
#define SR_DRQ 0x08
#define CMD_READ_SECT 0x20

static int ata_wait_bsy_clear(){
    for(int t=0;t<100000;t++){
        if(!(inb(R_STATUS)&SR_BSY)) return 0;
    }
    return -1;
}
static int ata_wait_drq_set(){
    for(int t=0;t<100000;t++){
        uint8_t s=inb(R_STATUS);
        if(!(s&SR_BSY) && (s&SR_DRQ)) return 0;
    }
    return -1;
}

int ata_init(void){
    outb(ATA_CTRL, 0x00);
    io_delay();
    return 0;
}

int ata_read28(uint32_t lba, uint8_t cnt, void* buf){
    if (cnt==0) return -1;
    if (ata_wait_bsy_clear()) return -1;

    outb(R_HDDEVSEL, 0xE0 | ((lba>>24)&0x0F)); // master, LBA
    outb(R_SECTCNT, cnt);
    outb(R_LBA0, (uint8_t)(lba & 0xFF));
    outb(R_LBA1, (uint8_t)((lba>>8)&0xFF));
    outb(R_LBA2, (uint8_t)((lba>>16)&0xFF));
    outb(R_CMD, CMD_READ_SECT);

    uint16_t* w=(uint16_t*)buf;
    for (int s=0; s<cnt; s++){
        if (ata_wait_drq_set()) return -1;
        for (int i=0;i<256;i++){
            uint16_t d; __asm__ __volatile__("inw %1,%0":"=a"(d):"Nd"(R_DATA));
            *w++ = d;
        }
    }
    return 0;
}

/* Отладочный дамп: readlba <LBA> <cnt> — печатаем первые 16 байт каждого сектора */
int ata_dump_lba28(uint8_t drv, uint32_t lba, uint32_t cnt){
    (void)drv; /* пока игнорируем drive: master */
    uint8_t buf[512];
    for (uint32_t i=0;i<cnt;i++){
        if (ata_read28(lba+i, 1, buf)!=0){ vga_puts("ATA read err\n"); return -1; }
        vga_puts("LBA "); vga_puthex(lba+i); vga_puts(": ");
        for (int j=0;j<16;j++){ uint8_t b=buf[j]; const char* h="0123456789ABCDEF";
            vga_putchar(h[(b>>4)&0xF]); vga_putchar(h[b&0xF]); vga_putchar(' ');
        }
        vga_puts("\n");
    }
    return 0;
}
