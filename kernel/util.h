#pragma once
#include <stddef.h>
#include <stdint.h>
void *memset(void*, int, size_t);
void *memcpy(void*, const void*, size_t);
size_t strlen(const char*);
int strcmp(const char*, const char*);
int strncmp(const char*, const char*, size_t);
