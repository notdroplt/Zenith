#include "formats.h"

static uint8_t calculate_checksum(uint16_t address,  uint64_t first, uint64_t second) {
    //                                        byte count ---\/ 
    uint8_t checksum = (address >> 8) + (address & 0xff) + 0x10;
    int i = 0;

    for (; i < 8; ++i) {
        // masking and then adding would have the same effect
        // so we save the cpu from doing an unnecessary mask
        checksum += (first + second) & 0xff ;
        first >>= 8;
        second >>= 8;
    }

    // sum of everything + checksum must return (uint8_t)0
    return -checksum;
}

int ihex_create_file(void * data, uint64_t data_size, const char * filename) {
    FILE * fp = NULL;
    uint16_t address = 0;
    uint64_t * castdata = data;
    unsigned int i = 0;
    
    if ((data_size & (int64_t)-8) != data_size) {
        fputs("data needs to be 8 byte aligned\n", stderr);
        return 1;
    }

    fp = fopen(filename, "we");

    if (!fp) { return 1;
}

    for (; i < (data_size >> 4) << 1; i += 2) {
        fprintf(fp, ":10%04X00%016lX%016lX%02X\n", address, castdata[i + 0], castdata[i + 1], calculate_checksum(address, castdata[i + 0], castdata[i + 1]));
        address += 0x10;
    }

    // data has an odd number of 
    if ((data_size >> 3) & 1) {
        fprintf(fp, ":10%04X00%016lX%016lX%02X\n", address, castdata[i], 0LU, calculate_checksum(address, castdata[i], 0));
    }
    
    //eof marker at end address
    fputs(":00FFFF0101", fp);

    fclose(fp);
    return 0;
}
