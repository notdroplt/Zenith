#include "typing.hpp"

using namespace Typing;

std::size_t RangeType::id() const
{
    if (this->type_id)
        return type_id;
    auto hash = std::hash<RangeSet>{}(this->superset);
    hash ^= std::hash<decltype(this->start)>{}(this->start);
    hash ^= std::hash<decltype(this->end)>{}(this->end);

    return hash;
}

std::partial_ordering RangeType::operator<=>(const RangeType &type) const
{
    if (auto cmp = superset <=> type.superset; cmp != 0)
        return static_cast<std::partial_ordering>(cmp);
    if (auto cmp = start <=> type.start; cmp != 0)
        return cmp;
    if (auto cmp = end <=> type.end; cmp != 0)
        return cmp;

    return std::partial_ordering::equivalent;
};

template <typename T>
std::optional<RangeType> RangeType::in_range(T value) const noexcept
{
    if ((std::is_integral_v<T> && this->superset == Reals) 
     || (std::is_floating_point_v<T> && this->superset == Integers)
     || this->start > value
     || this->end < value)
        return std::nullopt;

    return std::optional<RangeType>(*this);
}

returns<Type> RangeType::operate_with(const TypeOperations operation, std::optional<std::vector<Type>> operators)
{
    if (!operators)
    {
        switch (operation)
        {
        case Increment:
            if (start_is<int64_t>())
                return make_expected_unique<returns<Type>, RangeType>(get_start<int64_t>() + 1, get_end<int64_t>() + 1);

            if (start_is<uint64_t>())
                return make_expected_unique<returns<Type>, RangeType>(get_start<uint64_t>() + 1u, get_end<uint64_t>() + 1u);

            return make_expected_unique<returns<Type>, RangeType>(get_start<double>() + 1.0, get_end<double>() + 1);

        case Decrement:
            if (start_is<int64_t>())
                return make_expected_unique<returns<Type>, RangeType>(get_start<int64_t>() - 1, get_end<int64_t>() - 1);

            if (start_is<uint64_t>())
                return make_expected_unique<returns<Type>, RangeType>(get_start<uint64_t>() - 1u, get_end<uint64_t>() - 1u);

            return make_expected_unique<returns<Type>, RangeType>(get_start<double>() - 1.0, get_end<double>() - 1);

        case ChangeSignal:
            if (start_is<int64_t>())
                return make_expected_unique<returns<Type>, RangeType>(-get_end<int64_t>(), -get_start<int64_t>());

            if (start_is<uint64_t>())
                return make_expected_unique<returns<Type>, RangeType>(-get_end<uint64_t>(), -get_start<uint64_t>());

            return make_expected_unique<returns<Type>, RangeType>(-get_end<double>(), -get_start<double>());

        default:
            return make_unexpected<returns<Type>>("Type error", "current operation undefined");
        }
    }

    auto & first_op = operators->at(0);

    if (!is_convertible_to(*first_op))
        return make_unexpected<returns<Type>>("Type error", "current operation does not allow this type conversion");

    switch (operation)
    {
    case Add:
        if (start_is<int64_t>())
            return make_expected_unique<returns<Type>, RangeType>(get_start<int64_t>(), get_end<int64_t>() - 1);

        if (start_is<uint64_t>())
            return make_expected_unique<returns<Type>, RangeType>(get_start<uint64_t>() - 1u, get_end<uint64_t>() - 1u);

        return make_expected_unique<returns<Type>, RangeType>(get_start<double>() - 1.0, get_end<double>() - 1);
        break;
    
    default:
        break;
    }

    return make_unexpected<returns<Type>>("", "");
}