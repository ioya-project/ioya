#ifndef GUID_H_
#define GUID_H_

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} guid_t;

static inline char *guid_to_str(guid_t *guid)
{
    static char buffer[37];
    sprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", guid->data1,
            guid->data2, guid->data3, guid->data4[0], guid->data4[1], guid->data4[2],
            guid->data4[3], guid->data4[4], guid->data4[5], guid->data4[6], guid->data4[7]);
    return buffer;
}

#endif