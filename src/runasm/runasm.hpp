#pragma once
#ifndef ZENITH_RUNASM_H
#define ZENITH_RUNASM_H 1

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace runasm
{

    /**
     * @brief defines a register that isn't shadowed, so "light strikes on it"
     *
     */
    struct light_register
    {
        std::string name;  //!< architecture given register name
        uint16_t position; //!< position defined on the runasm file
        uint8_t size;      //!< bitsize
    };

    /**
     * @brief defines a shadowed register, so it
     *
     */
    struct shadow_register
    {
        std::string name;                        //!< architecure given register name
        std::vector<int16_t> shadowed_registers; //!< shadowed registers, in order
        uint16_t padding;                        // register padding size
    };

    using flags = std::unordered_map<std::string, uint8_t>;

    namespace HardwareController
    {
        struct offset
        {
            uint16_t index;
            uint16_t size;
        };

        class Format
        {
            uint64_t element_count;
            uint64_t instruction_bit_size;
            std::unordered_map<char, std::vector<int>> format_bits;
            std::unordered_map<char, std::string> aliases;
            uint8_t immediates;
            uint8_t registers;
        };

        enum Properties
        {
            bit_8 = 1 << 0,
            bit_16 = 1 << 1,
            bit_32 = 1 << 2,
            bit_64 = 1 << 3,
            bit_128 = 1 << 4,
            bit_256 = 1 << 5,
            bit_512 = 1 << 6,
            Vectorized = 1 << 7,
            Flags = 1 << 8,
            Threading = 1 << 9,
            SoftwareInterrupts = 1 << 10,
            HardwareInterrupts = 1 << 11,
            Paging = 1 << 12

        };
    }

    class InstructionSet
    {
        std::string name;
        std::vector<light_register> registers;
        std::vector<shadow_register> shade_registers;
        uint8_t width;

    public:
        std::string serialize(std::string filename);
        void get_serialized();
    };

}
#endif