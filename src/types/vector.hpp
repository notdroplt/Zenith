#pragma once
#ifndef ZENITH_TYPES_VECTOR_HPP
#define ZENITH_TYPES_VECTOR_HPP 1

#include "allocator.hpp"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <concepts>

namespace types
{
    namespace policies
    {
        /**
         * @brief Allocation methods allowed in some containers
         *
         * those values change the way containers that use "resize" work
         *
         * 1 - Addictively: every resize will @b add N elements to a container
         * 2 - Multiplicatively: every resize will @b multiply the container's size by N
         *
         */
        enum class AllocationMethod
        {
            Addictively = 0,     //!< indicate option 1
            Multiplicatively = 1 //!< indicate option 2
        };

        /**
         * @brief Defines types used 
         * @tparam T 
         */
        template <typename T>
        struct ContainerTypes {
            using value_type = T;
            using size_type = size_t;
            using difference_type = ptrdiff_t;
            using reference = T &;
            using pointer = T *;
            using const_pointer = const T *;
            using const_reference = const T &;
        };

        /**
         * @brief Dictates the internal workings of a Vector given template arguments
         * @tparam T Vector's type
         * @tparam _AllocationStacking how is the Vector supposed to be resized...
         * @tparam _AllocationAmount ... and how much
         * @tparam _Allocator which allocator to use
         * @tparam _InitialSize initial vector size (in elements)
         * @tparam _BlockSize amount to allocate for each block
         */
        template <
            typename T,
            typename _Allocator = types::nothrowHeapAllocator<T>,
            typename _Types = ContainerTypes<T>,
            AllocationMethod _AllocationStacking = AllocationMethod::Addictively,
            uintmax_t _AllocationAmount = 8,
            size_t _InitialSize = 8,
            size_t _BlockSize = sizeof(T)
            >
        struct VectorPolicy
        {
            using types = _Types;

            using Allocator = _Allocator;
            static auto constexpr AllocationStacking = _AllocationStacking;
            static auto constexpr AllocationAmount = _AllocationAmount;
            static auto constexpr InitialSize = _InitialSize;
            static auto constexpr BlockSize = _BlockSize;
        };
    };

    template <
        class T,
        class _Policy = typename policies::VectorPolicy<T>
    >
    class Vector
    {
        using policy = _Policy;
        using value_type = typename policy::types::value_type;

        using pointer = typename policy::types::pointer;
        using size_type = typename policy::types::size_type;
        using reference = typename policy::types::reference;
        using const_pointer = typename policy::types::const_pointer;
        using const_reference = typename policy::types::const_reference;
        using difference_type = typename policy::types::difference_type;

        using Allocator = typename policy::Allocator;
        std::unique_ptr<T[]> _ptr = nullptr;
        size_type _size = policy::InitialSize;
        size_type _capacity = policy::InitialSize;

        class iterator
        {
        public:
            using difference_type = typename Vector<T>::difference_type;
            using value_type = typename Vector<T>::value_type;
            using reference = typename Vector<T>::reference;
            using pointer = typename Vector<T>::pointer;
            using iterator_category = std::contiguous_iterator_tag; // or another tag

            iterator() = default;
            iterator(const iterator &) = default;
            iterator(const pointer &ptr) : _ptr(ptr) {}
            ~iterator() = default;

            iterator &operator=(const iterator &it)
            {
                this->_ptr = it._ptr;
                return *this;
            };
            constexpr bool operator==(const iterator &it) const { return this->_ptr == it._ptr; };
            constexpr auto operator!=(const iterator &it) const { return this->_ptr != it._ptr; };

            constexpr iterator &operator++()
            {
                ++this->_ptr;
                return *this;
            };
            constexpr iterator operator++(int)
            {
                auto tmp = *this;
                this->_ptr++;
                return tmp;
            }
            constexpr iterator &operator--()
            {
                --this->_ptr;
                return *this;
            };
            constexpr iterator operator--(int)
            {
                auto tmp = *this;
                this->_ptr--;
                return tmp;
            }
            constexpr iterator &operator+=(size_type off)
            {
                this->_ptr += off;
                return *this;
            };

            constexpr iterator operator+(size_type off) const { return this->_ptr + off; }
            constexpr friend iterator operator+(size_type off, const iterator &it) { return off + it._ptr; }
            constexpr iterator &operator-=(size_type off)
            {
                this->_ptr -= off;
                return *this;
            };
            constexpr iterator operator-(size_type off) const { return this->_ptr - off; }
            constexpr difference_type operator-(const iterator it) const { return this->_ptr - it._ptr; }

            constexpr reference operator[](size_type off) const { return this->_ptr[off]; }
            constexpr reference operator*() const { return *this->_ptr; };
            constexpr pointer operator->() const { return this->_ptr; };

            constexpr auto data() const { return this->_ptr; }

        private:
            pointer _ptr = nullptr;
        };

        class const_iterator
        {
        public:
            using difference_type = typename Vector<T>::difference_type;
            using value_type = typename Vector<T>::value_type;
            using reference = const typename Vector<T>::reference;
            using pointer = const typename Vector<T>::pointer;
            using iterator_category = std::contiguous_iterator_tag; // or another tag //or another tag

            const_iterator() = default;
            const_iterator(const const_iterator &) = default;
            const_iterator(const iterator &it) : _ptr(it.data()) {}
            const_iterator(const pointer &ptr) : _ptr(ptr) {}
            ~const_iterator() = default;

            const_iterator &operator=(const const_iterator &) = default;
            bool operator==(const const_iterator &it) const { return this->_ptr == it._ptr; };
            auto operator!=(const const_iterator &it) const { return this->_ptr != it._ptr; };

            constexpr const_iterator &operator+=(size_type off)
            {
                this->_ptr += off;
                return *this;
            };
            constexpr const_iterator operator+(size_type off) const { return this->_ptr + off; }
            constexpr friend const_iterator operator+(size_type off, const const_iterator &it) { return off + it._ptr; }
            constexpr const_iterator &operator-=(size_type off)
            {
                this->_ptr -= off;
                return *this;
            };
            constexpr const_iterator operator-(size_type off) const { return this->_ptr - off; }
            constexpr difference_type operator-(const const_iterator it) const { return this->_ptr - it._ptr; }

            constexpr reference operator[](size_type off) const { return this->_ptr[off]; }
            constexpr reference operator*() const { return *this->_ptr; };
            constexpr auto operator->() const { return this->_ptr; };

            constexpr auto data() const { return this->_ptr; }

        private:
            pointer _ptr = nullptr;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        Vector() : _ptr(std::make_unique(policy::InitialSize)) {
            
        }
        Vector(size_type size) : _ptr(std::make_unique(size)) {}

        constexpr iterator end() { return &this->_ptr[this->_size - 1]; }
        constexpr iterator begin() { return this->_ptr; }
        constexpr const_iterator end() const { return &this->_ptr[this->_size - 1]; }
        constexpr const_iterator cend() const { return &this->_ptr[this->_size - 1]; }
        constexpr const_iterator begin() const { return this->_ptr; }
        constexpr const_iterator cbegin() const { return this->_ptr; }

        constexpr reverse_iterator rend() { return &this->_ptr[this->_size - 1]; }
        constexpr reverse_iterator rbegin() { return this->_ptr; }
        constexpr const_reverse_iterator rend() const { return this->_ptr + this->_size; }
        constexpr const_reverse_iterator crend() const { return this->_ptr + this->_size; }
        constexpr const_reverse_iterator rbegin() const { return this->_ptr; }
        constexpr const_reverse_iterator crbegin() const { return this->_ptr; }

        constexpr reference back() { return this->_ptr[this->_size - 1]; }
        constexpr reference front() { return this->_ptr[0]; }
        constexpr const_reference back() const { return this->_ptr[this->_size - 1]; }
        constexpr const_reference front() const { return this->_ptr[0]; }

        constexpr reference operator[](uint64_t index) { return this->_ptr[index]; }
        constexpr const_reference operator[](uint64_t index) const { return this->_ptr[index]; }

        template <class... Args>
        void emplace_front(Args &&...args)
        {
            const auto old_size = this->size;
            auto ptr = this->resize(this->size - 1);
            if (!ptr) 
                return; // do NOT panic

            ptr[0] = T(std::forward(args)...);

            for(size_type i = 0; i < old_size; ++i) {
                ptr[i + 1] = std::move(this->_ptr[i]);
            }

            Allocator::deallocate(this->_ptr);

        }
        template <class... Args>
        void emplace_back(Args &&...); // optional
        void push_front(const T &);    // optional
        void push_front(T &&);         // optional
        void push_back(const T &);     // optional
        void push_back(T &&);          // optional
        void pop_front();              // optional
        void pop_back();               // optional

        template <class... Args>
        iterator emplace(const_iterator, Args &&...);    // optional
        iterator insert(const_iterator, const T &);      // optional
        iterator insert(const_iterator, T &&);           // optional
        iterator insert(const_iterator, size_type, T &); // optional
        template <class iter>
        iterator insert(const_iterator, iter, iter);               // optional
        iterator insert(const_iterator, std::initializer_list<T>); // optional
        iterator erase(const_iterator);                            // optional
        iterator erase(const_iterator, const_iterator);            // optional
        void clear();                                              // optional
        template <class iter>
        void assign(iter, iter);               // optional
        void assign(std::initializer_list<T>); // optional
        void assign(size_type, const T &);     // optional

        constexpr reference at(uint64_t index)
        {
            // instead of throwing (which would make the code non-freestanding), just return the last index
            if (index + 1 > this->_size)
            {
                index = this->_size - 1;
            }
            return this->_ptr[index];
        }

        constexpr size_type size() const { return this->_size; }
        constexpr size_type max_size() const { return this->_size; }
        constexpr bool empty() const { return this->_size == 0; }

        void swap(Vector<T> &other) const
        {
            auto begin = other.begin();
            for (auto &&element : *this)
            {
                std::swap(element, *begin);
            }
        }

        constexpr bool operator==(const Vector<T> &other) const { return iterator_equal(this->begin(), this->end(), other.begin()); }
        constexpr bool operator!=(const Vector<T> &other) const { return !iterator_equal(this->begin(), this->end(), other.begin()); }
    };
} // namespace types

#endif