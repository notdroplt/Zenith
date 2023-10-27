#include <iostream>
#include <typing/typing.hpp>

using namespace Typing;

int same(int, char ** ) {
    auto lower = std::rand();
    auto upper = std::rand();

    auto type_a = RangeType(lower, upper);
    auto type_b = RangeType(lower, upper);
    
    if (!(type_a == type_b)) {
        std::cerr << "FAILED: Types are not equal" << std::endl;
        return 1;
    }

    return 0;
}