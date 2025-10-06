#include "util.h"
void *memset(void* d,int v,size_t n){unsigned char* p=d;while(n--)*p++=(unsigned char)v;return d;}
void *memcpy(void* d,const void* s,size_t n){unsigned char* D=d;const unsigned char*S=s;while(n--)*D++=*S++;return d;}
size_t strlen(const char* s){size_t n=0;while(*s++)n++;return n;}
int strcmp(const char*a,const char*b){while(*a&&(*a==*b)){a++;b++;}return *(unsigned char*)a-*(unsigned char*)b;}
int strncmp(const char*a,const char*b,size_t n){while(n&&*a&&(*a==*b)){a++;b++;n--;}return n?*(unsigned char*)a-*(unsigned char*)b:0;}
