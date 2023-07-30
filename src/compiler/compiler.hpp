#pragma once
#ifndef ZENITH_COMPILER_H
#define ZENITH_COMPILER_H 1

#include <map>
#include <bitset>
#include <cstdint>
#include <variant>
#include <optional>
#include <parsing/parser.hpp>
#include <runners/supernova/supernova.h>

namespace Compiling
{
    class reg_status {
        std::bitset<32> _reg = std::bitset<32>();

        public:
        reg_status() = default;
        reg_status(std::bitset<32> regs) : _reg(regs){}
        returns<uint8_t> alloc(bool descending = false) {
            uint8_t i = 0;
            
            if (descending) {
                for (i = 31; i > 0; --i)        
                    if (this->_reg[i])            
                        break;
            } else 
                for (i = 1; i < 32; ++i)        
                    if (this->_reg[i])            
                        break;

            if (i) {
                this->_reg[i] = false;
                return make_expected<returns<uint8_t>>(i);
            }

            return make_unexpected<returns<uint8_t>>("assembler", "could not allocate register");

        }

        inline constexpr void free(uint8_t idx) { this->_reg[idx] = true; }

        inline constexpr void reset() { this->_reg = ~(0b111U << 29); }

        inline constexpr auto& status() { return this->_reg; }

        inline constexpr int8_t index() {
            for (int i = 0; i < 32; ++i) 
                if (this->_reg[i]) return i;
            return -1;
        }
    };

    using comp_status = std::variant<reg_status, uint64_t>;

    class Compiler final {
        /* variables / constants are **regions** on memory */
        struct region_entry {
            uint64_t allocated_at;
            bool constant;
            bool is_register;
        };

        struct lambda_entry {
            uint64_t allocated_at;
            std::map<std::string_view, region_entry> arguments;
            bool constant;
        };

        using context_types = std::variant<region_entry, lambda_entry>;     

        struct context {
            private:
            context * _parent;
            std::map<std::string, context_types> _symbols;
            public: 
            context(context * parent = nullptr, std::map<std::string, context_types> symbols = {}) {
                this->_parent = parent;
                this->_symbols = symbols;
            }

            void add_to_context(std::string_view name, context_type symbol) {
                this->_symbols[std::string(name)] = symbol;
            }

            returns<context_types> operator[](std::string_view name) {
                auto lookup = std::string(name);

                if (this->_symbols.contains(lookup))
                    return make_expected<returns<context_types>, context_types>(this->_symbols[lookup]);

                if (this->_parent)
                    return (*this->_parent)[name];

                std::string error = std::string("no variable of name \"") + std::string(name) + "\" found";

                return make_unexpected<returns<context_types>>("compiling", error);
            }
        };

        private:
        std::vector<Parsing::nodep> _nodes;
        std::vector<std::string_view> _strings;
        
        uint64_t calculate_string_byte_offset(uint64_t index) {
            uint64_t acc = 0;
            for (int i = 0; i < index - 1; ++i)
                acc = this->_strings[i].length() + 1;

            return acc;            
        }

        uint64_t get_string(std::string_view str) {
            for(int i = 0; i < this->_strings.size(); ++i)
                if (this->_strings[i] == str)
                    return calculate_string_byte_offset(i);
            
            return -1LLU;
        }

        void return_references();

        context _global_context = context();

        // reference to current compiling context, changes inside lambdas
        context & _current_context = _global_context;

        std::vector<uint64_t> _instructions;
        uint64_t _dot = 0x1000;
        uint64_t _root_index = 0;
        uint64_t _entry_point = 0;
        
        reg_status _registers;
        void append_instruction(instruction_t instruction) {
            this->_instructions.push_back(instruction.value);
            this->_dot += 8;
        }

        returns<comp_status> compile_number(const Parsing::nodep &number);
        returns<comp_status> compile_unary(const Parsing::nodep &unary);
        returns<comp_status> compile_binary(const Parsing::nodep &binary, bool jumping);
        returns<comp_status> compile_identifier(const Parsing::nodep &identifier);
        returns<comp_status> compile_lambda(const Parsing::nodep &lambda);

        returns<comp_status> compile(const Parsing::nodep &node);

        public:
        Compiler(std::vector<Parsing::nodep> nodes);
    };



} // namespace Compiling


#endif 