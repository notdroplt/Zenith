#pragma once
#ifndef ZENITH_RUNASM_H
#define ZENITH_RUNASM_H 1

#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cstdint.h>

struct light_register {
    std::string name;
    uint16_t position;
    uint8_t size;
    bool shadowed;
};

struct shadow_register {
    std::string name;
    std::vector<int16_t> shadowed_registers;
    uint16_t padding;
};

using flags = std::unordered_map<std::string, uint8_t>;
using format = std::vector<char> layout;

class InstructionSet {
    std::string name;
    std::vector<light_register> registers;
    std::vector<shadow_register> shade_registers;
    uint8_t width;
    public:

    std::string serialize(std::string filename);
    void serialized(std::string filename);

    
};

#endif 