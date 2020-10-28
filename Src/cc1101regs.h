#ifndef CC1101_REGS_H
#define CC1101_REGS_H

#include <stdint.h>


//0x08: PKTCTRL0 – Packet Automation Control
typedef union
{
    uint8_t Value;

    struct
    {
        unsigned LENGTH_CONFIG:2;
        unsigned CRC_EN:1;
        unsigned reserved:1;
        unsigned PKT_FORMAT:2;
        unsigned WHITE_DATA:1;
        unsigned reserved2:1;

    }u;

} PKTCTRL0;


//0x07: PKTCTRL1 – Packet Automation Control

typedef union
{
    uint8_t Value;

    struct
    {
        unsigned ADR_CHK:2;
        unsigned APPEND_STATUS:1;
        unsigned CRC_AUTOFLUSH:1;
        unsigned reserved:1;
        unsigned PQT:3;
    }u;

} PKTCTRL1;


#endif




