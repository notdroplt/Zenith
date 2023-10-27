#pragma once
#ifndef ZENITH_TYPING_HPP
#define ZENITH_TYPING_HPP 1

#include "../platform.hpp"
#include <vector>
#include <compare>
#include <string>
#include <optional>
#include <concepts>
#include <type_traits>
#include <variant>
#include <memory>
#include <unordered_map>

namespace Typing {

    class MetaType {
        public:

        /**
         * @brief implement operation on types
         * 
         * operations don't have constraints in which types they should return or not
         */
        enum TypeOperations {
            Add, //!< Type + Type -> Type
            Increment, //!< Type++ -> Type
            Subtract, //!< Type - Type -> Type
            Decrement, //!< Type-- -> Type
            Multiplicate, //!< Type * Type -> Type
            ShiftLeft, //!< Type << Type -> Type
            Division, //!< Type / Type -> Type
            ShiftRight, //!< Type >> Type -> Type
            ChangeSignal, //!< -Type -> Type
            Index, //!< Type[Type] -> Type
            Call, //!< Type(Type, ...) -> Type
            Reference, //!< &Type -> Type
            Dereference, //!< *Type -> Type
            Compare, //!< Type <=> Type -> Type
        };

        /**
         * @brief generate an id for a specified type
         * 
         * @note a call to this function should generate the same result regardless of the type name, but shouldn't 
         * return the same value if the compiler is run twice
         * 
         * @returns the type id 
         */
        virtual std::size_t id() const { return 0; };

        /* some type traits */

        /**
         * @brief define when a type is \<void>
         * 
         * @returns true for derived class @ref VoidType
         * @returns false for all other cases
         */
        virtual bool is_void() const noexcept { return false; };

        /**
         * @brief define when a type is a <range> and also an integer
         * 
         * @returns true for any variation of @ref RangeType that is also an integer
         * @returns false for all other cases
         */
        virtual bool is_integral() const noexcept { return false; };

        /**
         * @brief define when a type is a \<range> but covers the real number line
         * 
         * Obviously, not the @a entire number line, as precision is not infinte
         * 
         * @returns true for any variation of @ref RangeType that has its superset = Reals
         * @returns false for all other cases 
         */
        virtual bool is_floating_point() const noexcept { return false; };

        /**
         * @brief define when a type is an array of any other type
         * 
         * @note all arrays @a should @a be pointers, but the opposite might not be true, that's why
         * the function doesn't return a call to is_pointer
         *
         * @returns true when type is an array
         * @returns false for all other cases
         */
        virtual bool is_array() const noexcept { return false; };

        /**
         * @brief define when a type comes from an enumeration
         * 
         * @warning not yet implemented but a feature to come soon
         *
         * @returns true when type is an enum
         * @returns false for all other cases
         */
        virtual bool is_enum() const noexcept { return false; };

        /**
         * @brief define when a type is an union of other types
         * 
         * @returns true when type is an union
         * @returns false for all other cases
         */
        virtual bool is_union() const noexcept { return false; };

        /**
         * @brief define when a type is a structured type
         * 
         * @returns true when type is a structure
         * @returns false for all other cases
         */
        virtual bool is_struct() const noexcept { return false; };

        /**
         * @brief define when a type is a function type
         * 
         * @returns true for functions
         * @returns false for all other cases
        */
        virtual bool is_function() const noexcept { return false; };

        /**
         * @brief define when a type is a pointer type
         * 
         * @returns true for pointers
         * @returns false for all other cases
        */
        virtual bool is_pointer() const noexcept { return false; };

        /**
         * @brief define if a type is arithmetic
         * 
         * @returns true for integrals and floats
         * @returns false for all other cases
        */
        bool is_arithmetic() const noexcept { return is_integral() || is_floating_point(); }

        /**
         * @brief define a fundamental type
         * 
         * @returns true for any type that is on any kind aggregate
         * @returns false for all other cases
        */
        bool is_fundamental() const noexcept { return is_arithmetic() || is_void(); }
        bool is_scalar() const noexcept { return is_arithmetic() || is_enum() || is_pointer(); }
        bool is_object() const noexcept { return is_scalar() || is_array() || is_union() || is_struct(); }
        bool is_compound() const noexcept { return !is_fundamental(); }

        virtual bool is_mutable() const noexcept { return false; };
        virtual bool is_volatile() const noexcept { return false; };
        virtual bool is_literal() const noexcept { return false; };
        virtual bool is_signed() const noexcept { return false; };
        virtual bool is_unsigned() const noexcept { return false; };

        virtual bool is_same(const MetaType & type) const noexcept { return this->id() == type.id(); }
        virtual bool is_base_of(const MetaType &) const noexcept { return false; }

        virtual bool is_convertible_to(const MetaType &) const noexcept { return false; }
        virtual bool is_loose_convertible_to(const MetaType &) const noexcept { return false; }

        bool operator==(const MetaType & type) const noexcept { return this->is_same(type); }
        bool operator>(const MetaType & type) const noexcept { return this->is_base_of(type); }
        bool operator<(const MetaType & type) const noexcept { return type.is_base_of(*this); }

        template <typename T> requires (std::is_base_of_v<MetaType, T>)
        constexpr T as() { return static_cast<T>(*this); }

        virtual returns<std::unique_ptr<MetaType>> operate_with(const TypeOperations operation, std::optional<std::vector<std::unique_ptr<MetaType>>> operators) {
            (void)operation;
            (void)operators;
            return make_unexpected<returns<std::unique_ptr<MetaType>>>("Type error", "Cannot operate on a class that has no operators defined");
        }

        virtual ~MetaType() = default;

        static std::unordered_map<std::size_t, std::unique_ptr<MetaType>&> types;
    };

    //!< Base class pointer type
    using Type = std::unique_ptr<MetaType>;

    /**
     * @brief a type to represent nothing
    */
    class VoidType final : public MetaType {
        VoidType() {}
        virtual bool is_void() const noexcept override { return true; }
        virtual std::size_t id() const override { return -1; }

    };
    class RangeType final : public MetaType {        
        public:

        //!< possible delimiter types
        using Delimiter = std::variant<int64_t, uint64_t, double>;

        /**
         * @brief Mathematical supersets of range types 
         */
        enum RangeSet {
            Integers, //!< Represents integer numbers only
            Reals, //!< Represents all real numbers
            //!< TODO: complex numbers
        };

        /**
         * @brief overriding base class as ranges can be integers
         * 
         * @return 
         */
        virtual bool is_integral() const noexcept override {
            return this->superset == Integers;
        }

        virtual bool is_floating_point() const noexcept override {
            return this->superset == Reals;
        }


        /**
         * @brief generate id for type
         * 
         * @param type type to genererate id for
         * @return type id
         */
        virtual std::size_t id() const override;

        /**
         * @brief Default constructor for a range type
         * @param superset number superset 
         
         */
        RangeType(RangeSet superset = RangeSet::Integers) : superset(superset), type_id(id()) { }

        /**
         * @brief generate a range that is either $$[0, border] \cap \mathbb{Z}\f$$ 
         * @tparam T 
         */
        template <std::signed_integral T>  
        constexpr RangeType(T border) : RangeType(RangeSet::Integers) {
            this->start.emplace<int64_t>(border > 0 ? 0 : border);
            this->end.emplace<int64_t>(border < 0 ? 0 : border); 
        }
        
        template <std::unsigned_integral T> 
        constexpr RangeType(T border) : RangeType(RangeSet::Integers) {
            this->start.emplace<uint64_t>(0);
            this->end.emplace<uint64_t>(border); 
        }

        template <std::floating_point T>
        constexpr RangeType(T border) : RangeType(RangeSet::Reals) {
            this->start.emplace<double>(border > 0.0 ? 0.0 : border);
            this->end.emplace<double>(border < 0.0 ? 0.0 : border);  
        }

        template <std::signed_integral T> 
        constexpr RangeType(T start, T end) : RangeType(RangeSet::Integers) {
            this->start.emplace<int64_t>(start);
            this->end.emplace<int64_t>(end); 
        }
        
        template <std::unsigned_integral T> 
        constexpr RangeType(T start, T end) : RangeType(RangeSet::Integers) {
            this->start.emplace<uint64_t>(start);
            this->end.emplace<uint64_t>(end); 
        }

        template <std::floating_point T>
        constexpr RangeType(T start, T end) : RangeType(RangeSet::Reals) {
            this->start.emplace<double>(start);
            this->end.emplace<double>(end);  
        }

        bool operator==(const RangeType & type) const noexcept { return this->type_id == type.type_id; }

        std::partial_ordering operator<=>(const RangeType & type) const;

        template<typename T> 
        std::optional<RangeType> in_range(T value) const noexcept;

        /**
         * @brief defines if a type is smaller than other
         *
         * @param subtype possible subtype
         * @returns true in case current type contains subtype
         * @returns false otherwise
         */
        bool is_supertype_of(const RangeType & subtype) const noexcept { return (*this <=> subtype) > 0; }
        
        /**
         * @brief checks whether a type is bigger than other
         * 
         * @param supertype possible supertype
         * @returns true in case current type is contained in supertype
         * @returns false otherwise
         */
        bool is_subtype_of(const RangeType & supertype) const noexcept { return *this <= supertype ; }

        /**
         * @brief applies an operation to a type
         * 
         * @param operation type of operation
         * @param operators if any, what to operate with
         * @returns Type for the resulting type if the operation is permitted
         * @returns TypeError otherwise
         */
        virtual returns<Type> operate_with(const TypeOperations operation, std::optional<std::vector<Type>> operators) override;

        /**
         * @brief 
        */
        template <typename T>
        constexpr T get_start() const { return std::get<T>(start); }

        template <typename T>
        constexpr T get_end() const { return std::get<T>(end); }

        template <typename T>
        constexpr bool start_is() const noexcept { return std::get_if<T>(&this->start) != nullptr; }

        template <typename T>
        constexpr bool end_is() const noexcept { return std::get_if<T>(&this->end) != nullptr; }
        private: 
            RangeSet superset;
            Delimiter start;
            Delimiter end;

            std::size_t type_id = 0;
    };

    class PointerType : public MetaType {
        int _dimensions;
        Type _pointer;

        public: 
        PointerType(Type pointer , int);

        virtual bool is_base_of(const MetaType & type) const noexcept
        { return type == *_pointer; };
    };

    class FunctionType final : public MetaType {
        private: 
            std::optional<std::vector<Type>> arguments;
            Type rettype;

        public:

        FunctionType(std::optional<std::vector<Type>> args, Type return_type) : arguments(std::move(args)), rettype(std::move(return_type)) { }

        virtual bool is_function() const noexcept override { return true; }
        virtual bool is_pointer() const noexcept override { return true; }
    };
}

#endif 