#pragma once

#ifndef ZENITH_TYPES_ARRAY_HPP
#define ZENITH_TYPES_ARRAY_HPP 1

#include <cstddef>
#include <cstdint>
#include <iterator>

namespace types
{

    template <typename It1, typename It2>
    constexpr bool iterator_equal(It1 if1, It1 il1, It2 if2)
    {
        for (; if1 != il1; ++if1, (void)++if2)
        {
            if (*if1 != *if2)
            {
                return false;
            }
        }
        return true;
    }

    template <typename T, size_t S = 0>
    class Array final
    {

    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = T &;
        using pointer = T *;
        using const_pointer = const T *;
        using const_reference = const T &;

        class iterator
        {
        public:
            using difference_type = Array<T, S>::difference_type;
            using value_type = Array<T, S>::value_type;
            using reference = Array<T, S>::reference;
            using pointer = Array<T, S>::pointer;
            using iterator_category = std::contiguous_iterator_tag; // or another tag

            iterator() = default;
            iterator(const iterator &) = default;
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
            using difference_type = Array<T, S>::difference_type;
            using value_type = Array<T, S>::value_type;
            using reference = const Array<T, S>::reference;
            using pointer = const Array<T, S>::pointer;
            using iterator_category = std::contiguous_iterator_tag; // or another tag //or another tag

            const_iterator() = default;
            const_iterator(const const_iterator &) = default;
            const_iterator(const iterator &it) : _ptr(it.data()) {}
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

        constexpr iterator end() { return &this->_ptr[S - 1]; }
        constexpr iterator begin() { return this->_ptr; }
        constexpr const_iterator end() const { return &this->_ptr[S - 1]; }
        constexpr const_iterator cend() const { return &this->_ptr[S - 1]; }
        constexpr const_iterator begin() const { return this->_ptr; }
        constexpr const_iterator cbegin() const { return this->_ptr; }

        constexpr reverse_iterator rend() { return &this->_ptr[S - 1]; }
        constexpr reverse_iterator rbegin() { return this->_ptr; }
        constexpr const_reverse_iterator rend() const { return this->_ptr + S; }
        constexpr const_reverse_iterator crend() const { return this->_ptr + S; }
        constexpr const_reverse_iterator rbegin() const { return this->_ptr; }
        constexpr const_reverse_iterator crbegin() const { return this->_ptr; }

        constexpr reference back() { return this->_ptr[S - 1]; }
        constexpr reference front() { return this->_ptr[0]; }
        constexpr const_reference back() const { return this->_ptr[S - 1]; }
        constexpr const_reference front() const { return this->_ptr[0]; }

        constexpr reference operator[](uint64_t index) { return this->_ptr[index]; }
        constexpr const_reference operator[](uint64_t index) const { return this->_ptr[index]; }

        constexpr reference at(uint64_t index)
        {
            // instead of throwing (which would make the code non-freestanding), just return the last index
            if (index + 1 > S)
            {
                index = S - 1;
            }
            return this->_ptr[index];
        }

        constexpr size_type size() const { return S; }
        constexpr size_type max_size() const { return S; }
        constexpr bool empty() const { return S == 0; }

        void swap(Array<T, S> &other) const
        {
            auto begin = other.begin();
            for (auto &&element : *this)
            {
                std::swap(element, *begin);
            }
        }

        constexpr bool operator==(const Array<T, S> &other) const { return iterator_equal(this->begin(), this->end(), other.begin()); }
        constexpr bool operator!=(const Array<T, S> &other) const { return !iterator_equal(this->begin(), this->end(), other.begin()); }

    private:
        size_t _size = S;
        T _ptr[S];
    };
}

#endif