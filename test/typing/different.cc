#include <iostream>
#include <typing/typing.hpp>
#include <random>

using namespace Typing;

int different(int, char ** ) {
    auto lower_a = std::rand();
    auto lower_b = std::rand();

    auto upper_a = std::rand() + lower_a;
    auto upper_b = std::rand() + lower_b;

    while (lower_a == lower_b
        && upper_a == upper_b )
    {
        lower_b = std::rand();
        upper_b = std::rand() + lower_b;
    }

    auto type_a = RangeType(lower_a, upper_a);
    auto type_b = RangeType(lower_b, upper_b);
    
    if (!(type_a != type_b)) {
        std::cerr << "FAILED: Types are not equal" << std::endl;
        return 1;
    }

    if (type_a != type_a) {
        std::cerr << "Type A is not equal to itself" << std::endl;
        return 1;
    }

    return 0;
}