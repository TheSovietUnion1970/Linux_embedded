#include <stdint.h>
#include <stdio.h>

/* Compute VFAT LFN checksum for an 8.3 short name (11 bytes) */
uint8_t vfat_lfn_checksum(const uint8_t sfn[11]) {
    uint8_t chk = 0;
    for (int i = 0; i < 11; ++i) {
        /* rotate-right by 1, then add next byte */
        chk = ((chk & 1) ? 0x80 : 0) + (chk >> 1) + sfn[i];
    }
    return chk;
}

int main(void) {
    /* "WIFI" + 7 spaces = "WIFI       " (exactly 11 bytes) */
    uint8_t sfn[11] = { 'W','I','F','I',' ',' ',' ',' ',' ',' ',' ' };
    printf("LFN checksum = 0x%02X\n", vfat_lfn_checksum(sfn));  // -> 0x12
    return 0;
}