#include "formats.h"

static uint8_t calculate_checksum(uint16_t address,  uint64_t first, uint64_t second) {
    uint64_t sum = first + second;
    uint8_t checksum = sum + 0x10;

    checksum += sum >> 0x08;
    checksum += sum >> 0x10;
    checksum += sum >> 0x18;
    checksum += sum >> 0x20;
    checksum += sum >> 0x28;
    checksum += sum >> 0x30;
    checksum += sum >> 0x38;

    // sum of everything + checksum must return (uint8_t)0
    return -(checksum + (address >> 8) + (address & 0xff));
}

int ihex_create_file(void * data, uint64_t data_size, const char * filename) {
    FILE * fp = NULL;
    uint16_t address = 0;
    uint64_t * castdata = data;
    unsigned int i = 0;
    
    if ((data_size & -8LLU) != data_size) {
        fputs("data needs to be 8 byte aligned\n", stderr);
        return 1;
    }

    fp = fopen(filename, "we");

    if (!fp) return 1;

    for (; i < (data_size >> 4) << 1; i += 2) {
        fprintf(fp, ":10%04X00%016lX%016lX%02X\n", address, castdata[i + 0], castdata[i + 1], calculate_checksum(address, castdata[i + 0], castdata[i + 1]));
        address += 0x10;
    }

    // data has an odd number of bytes
    if ((data_size >> 3) & 1) {
        fprintf(fp, ":10%04X00%016lX%016lX%02X\n", address, castdata[i], 0LU, calculate_checksum(address, castdata[i], 0));
    }
    
    //eof marker at end address
    fputs(":00FFFF0101", fp);

    fclose(fp);
    return 0;
}
