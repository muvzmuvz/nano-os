#include "util.h"

void k_strlcpy(char* dst, const char* src, int n){
    if (!dst || n<=0) return;
    int i=0;
    if (src){
        while (src[i] && i < n-1) { dst[i] = src[i]; ++i; }
    }
    dst[i] = 0;
}

size_t kstrlen(const char* s){
    size_t n=0; if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

int kstrcmp(const char* a, const char* b){
    while (*a && (*a == *b)) { ++a; ++b; }
    return (unsigned char)*a - (unsigned char)*b;
}

int kstrncmp(const char* a, const char* b, size_t n){
    for (size_t i=0;i<n;i++){
        unsigned char ca=a[i], cb=b[i];
        if (ca!=cb || ca==0 || cb==0) return (int)ca - (int)cb;
    }
    return 0;
}

void* kmemcpy(void* dst, const void* src, size_t n){
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i=0;i<n;i++) d[i]=s[i];
    return dst;
}

void* kmemset(void* dst, int v, size_t n){
    unsigned char* d = (unsigned char*)dst;
    unsigned char b = (unsigned char)v;
    for (size_t i=0;i<n;i++) d[i]=b;
    return dst;
}

int k_atoi(const char* s){
    if (!s) return 0;
    int sign = 1;
    while (*s==' ' || *s=='\t') ++s;
    if (*s=='+' || *s=='-'){ if (*s=='-') sign=-1; ++s; }
    int x=0;
    while (isdigit_c(*s)){ x = x*10 + (*s - '0'); ++s; }
    return sign*x;
}
