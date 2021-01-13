// Implemented as per https://zerosoft.zophar.net/ips.php

#include <stdio.h>
#include <string.h>

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

#define BUF_CHECK(buf, buf_end) \
    if (buf >= buf_end) { printf("ERROR: Buffer overflow"); return 0; }

#define BYTE3_TO_UINT(bp) \
     (((unsigned int)(bp)[0] << 16) & 0x00FF0000) | \
     (((unsigned int)(bp)[1] << 8) & 0x0000FF00) | \
     ((unsigned int)(bp)[2] & 0x000000FF)

#define BYTE2_TO_UINT(bp) \
    (((unsigned int)(bp)[0] << 8) & 0xFF00) | \
    ((unsigned int) (bp)[1] & 0x00FF)

int ips_patch(
    char *buf_rom, int len_rom,
    char *buf_ips, int len_ips
) {
    int size_out = 0;
    char *buf_ips_end = buf_ips + len_ips;
    char *buf_rom_end = buf_rom + len_rom;

    BUF_CHECK(buf_ips + 5, buf_ips_end);
    if (strncmp("PATCH", buf_ips, 5) != 0)
        return 0;
    buf_ips += 5;

    while (strncmp("EOF", buf_ips, 3) != 0) {
        BUF_CHECK(buf_ips + 3, buf_ips_end);
        unsigned int offset = BYTE3_TO_UINT(buf_ips);
        buf_ips += 3;
        BUF_CHECK(buf_ips + 2, buf_ips_end);
        unsigned int size = BYTE2_TO_UINT(buf_ips);
        buf_ips += 2;

        // RLE encoding
        if (size == 0) {
            BUF_CHECK(buf_ips + 2, buf_ips_end);
            unsigned int RLE_size = BYTE2_TO_UINT(buf_ips);
            buf_ips += 2;
            BUF_CHECK(buf_ips, buf_ips_end);
            unsigned char value = *buf_ips;
            buf_ips++;
            size_out = MAX(size_out, offset + RLE_size);
            BUF_CHECK(buf_rom + offset + RLE_size, buf_rom_end);
            memset(buf_rom + offset, value, RLE_size);
        } else {
            size_out = MAX(size_out, offset + size);
            BUF_CHECK(buf_ips + size, buf_ips_end);
            BUF_CHECK(buf_rom + offset + size, buf_rom_end);
            memcpy(buf_rom + offset, buf_ips, size);
            buf_ips += size;
        }
    }

    return size_out;
}