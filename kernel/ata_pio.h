#pragma once
#include <stdint.h>
int  ata_init(void);                                // 0=ok
int  ata_read28(uint32_t lba, uint8_t cnt, void*);  // 0=ok, cnt∈[1..128] (по 512 байт)
