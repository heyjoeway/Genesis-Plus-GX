#ifndef __IPS_H__
#define __IPS_H__

int ips_patch(
    char *buf_rom, int len_rom,
    char *buf_ips, int len_ips
);

#endif