#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

template <typename T, typename Allocator = std::allocator<T>>
class vector {
public:
    // Member types
    using value_type = T;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;

    // pointers
    using pointer = T*;
    using const_pointer = const T*;

    // iterators
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Ctors

    explicit vector(const Allocator& allocator = Allocator()) : size_(0), capacity_(0), data_(nullptr), alloc(allocator) {}

    vector(size_type size, const T& value = T(), const Allocator& allocator = Allocator()) : size_(size), capacity_(0), data_(nullptr), alloc(allocator) {
        ResizeArray(size);

        for (size_type i = 0; i < size; ++i) {
            traits_t::construct(alloc, data_ + i, value);
        }
    }

    vector(const vector& other, const Allocator& allocator = Allocator()) : alloc(allocator) {
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = traits_t::allocate(alloc, capacity_);
    
        for (size_type i = 0; i < size_; ++i) {
            traits_t::construct(alloc, data_ + i, other[i]);
        }
    }

    vector(vector&& other, const Allocator& allocator = Allocator()) : alloc(allocator) {
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = other.data_;
        other.data_ = nullptr;
    }

    vector(iterator begin, iterator end) {
        size_ = end - begin;
        capacity_ = size_;
        data_ = traits_t::allocate(alloc, capacity_);

        for (size_type i = 0; i < size_; ++i) {
            traits_t::construct(alloc, data_ + i, *(begin++));
        }
    }

    vector(std::initializer_list<T> init, const Allocator& allocator = Allocator()) : size_(init.size()), capacity_(init.size()), alloc(allocator) {
        data_ = traits_t::allocate(alloc, capacity_);
        size_type offset = 0;
        for (const T& val : init) {
            traits_t::construct(alloc, data_ + (offset++), std::move(val));
        }
    }

    // operators

    vector& operator=(const vector& other) {
        if (data_ != nullptr) {
            for (size_type i = 0; i < size_; ++i) {
                traits_t::destroy(alloc, data_ + i);
            }
            traits_t::deallocate(alloc, data_, capacity_);
        }

        size_ = other.size();
        capacity_ = other.capacity();

        data_ = traits_t::allocate(alloc, capacity_);
        for (size_type i = 0; i < size_; ++i) {
            traits_t::construct(alloc, data_ + i, other[i]);
        }

        return *this;
    }

    vector& operator=(vector&& other) {
        if (data_ != nullptr) {
            for (size_type i = 0; i < size_; ++i) {
                traits_t::destroy(alloc, data_ + i);
            }
            traits_t::deallocate(alloc, data_, capacity_);
        }

        size_ = other.size();
        capacity_ = other.capacity();

        data_ = other.data_;
        other.data_ = nullptr;

        return *this;
    }

    allocator_type get_allocator() const {
        return alloc;
    }

    // Destructor

    ~vector() {
        if (data_ == nullptr) {
            return;
        }

        for (size_type i = 0; i < size_; ++i) {
            traits_t::destroy(alloc, data_ + i);
        }

        traits_t::deallocate(alloc, data_, capacity_);
    }

    // Element access

    T& at(size_type pos) {
        if (pos < 0 || pos >= size_) {
            throw std::out_of_range("No such pos");
        }
        return data_[pos];
    }

    const T& at(size_type pos) const {
        if (pos < 0 || pos >= size_) {
            throw std::out_of_range("No such pos");
        }
        return data_[pos];
    }

    T& operator[](size_type pos) {
        return data_[pos];
    }

    const T& operator[](size_type pos) const {
        return data_[pos];
    }

    T& front() {
        return data_[0];
    }

    const T& front() const {
        return data_[0];
    }

    T& back() {
        return data_[size_ - 1];
    }

    const T& back() const {
        return data_[size_ - 1];
    }

    T* data() {
        return data_;
    }

    const T* data() const{
        return data_;
    }

    // iterators

    iterator begin() noexcept {
        return data_;
    }

    const_iterator begin() const noexcept {
        return data_;
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return begin() + size_;
    }

    const_iterator end() const noexcept {
        return begin() + size_;
    }

    const_iterator cend() const noexcept {
        return end();
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(data_ + size_);
    }

    const_reverse_iterator rbegin() const noexcept {
        return rbegin();
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(data_);
    }

    const_reverse_iterator rend() const noexcept {
        return rend();
    }

    // Capacity

    bool empty() const {
        return size_ == 0;
    }

    size_type size() const {
        return size_;
    }

    size_type max_size() const {
        return std::numeric_limits<difference_type>::max();
    }

    void reserve(size_type new_cap) {
        if (new_cap <= capacity_) {
            return;
        }

        if (new_cap > max_size()) {
            throw std::length_error("Too much");
        }

        ResizeArray(new_cap);
    }

    size_type capacity() const {
        return capacity_;
    }

    void shrink_to_fit() {
        ResizeArray(size_);
    }

    // Modifiers

    void clear() {
        for (size_type i = 0; i < size_; ++i) {
            traits_t::destroy(alloc, data_ + i);
        }
        size_ = 0;
    }

    iterator insert(iterator pos, const T& value) {
        const difference_type diff = pos - begin();

        if (pos == end()) {
            push_back(value);
            pos = begin() + diff;
            return pos;
        }

        ExpandIfNeed();

        pos = begin() + diff;
        iterator curr_end = begin() + size_;

        if (pos < begin() || pos > curr_end) {
            throw std::out_of_range("Invalid insert pos");
        }

        traits_t::construct(alloc, curr_end, *(curr_end - 1));
        for (iterator it = curr_end - 1; it != pos; --it) {
            *it = std::move(*(it - 1));
        }

        *pos = value;
        ++size_;
    
        return pos;
    }

    iterator insert(iterator pos, T&& value) {
        const difference_type diff = pos - begin();

        if (pos == end()) {
            push_back(std::move(value));
            return begin() + diff;
        }
        
        ExpandIfNeed();

        pos = begin() + diff;
        iterator curr_end = end();

        if (pos < begin() || pos > end()) {
            throw std::out_of_range("Invalid insert pos");
        }

        traits_t::construct(alloc, curr_end, std::move(*(curr_end - 1)));
        for (iterator it = curr_end - 1; it != pos; --it) {
            *it = std::move(*(it - 1));
        }

        *pos = std::move(value);
        ++size_;
    
        return pos;
    }

    template< class... Args >
    iterator emplace(iterator pos, Args&&... args) {
        const difference_type diff = pos - begin();

        if (pos == end()) {
            emplace_back(std::forward<Args>(args)...);
            return begin() + diff;
        }

        ExpandIfNeed();

        pos = begin() + diff;
        iterator curr_end = begin() + size_;

        if (pos < begin() || pos > curr_end) {
            throw std::out_of_range("Invalid insert pos");
        }

        traits_t::construct(alloc, curr_end, std::move(*(curr_end - 1)));
        for (iterator it = curr_end - 1; it != pos; --it) {
            *it = std::move(*(it - 1));
        }

        traits_t::destroy(alloc, pos);
        traits_t::construct(alloc, pos, std::forward<Args>(args)...);
        ++size_;

        return pos;
    }

    iterator erase(iterator pos) {
        for (iterator it = pos; it != end() - 1; ++it) {
            traits_t::destroy(alloc, it);
            traits_t::construct(alloc, it, std::move(*(it + 1)));
        }
        traits_t::destroy(alloc, end() - 1);
        --size_;
        return pos;
    }

    void push_back(const T& value) {
        ExpandIfNeed();

        traits_t::construct(alloc, end(), value);
        ++size_;
    }

    void push_back(T&& value) {
        ExpandIfNeed();

        traits_t::construct(alloc, end(), std::move(value));
        ++size_;
    }

    template< class... Args >
    void emplace_back(Args&&... args) {
        ExpandIfNeed();
        traits_t::construct(alloc, end(), std::forward<Args>(args)...);
        ++size_;
    }

    void pop_back() {
        traits_t::destroy(alloc, end() - 1);
        --size_;
    }

    void resize(size_type new_size) {
        size_type old_size = size_;
        ResizeArray(new_size);
        for (size_type i = old_size; i < new_size; ++i) {
            traits_t::construct(alloc, data_ + i, std::move(T()));
            ++size_;
        }
    }

    void swap(vector& other) {
        std::swap(data_, other.data_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
        std::swap(alloc, other.alloc);
    }

private:
    using traits_t = std::allocator_traits<allocator_type>;

    size_type size_;
    size_type capacity_;
    allocator_type alloc;
    
    T* data_;

    void ResizeArray(size_type new_cap) {
        T* new_data = traits_t::allocate(alloc, new_cap);

        // FIXME move ctor should be noexcept here, else should use copy ctor for
        if (new_cap < size_) {
            for (size_type i = 0; i < new_cap; ++i) {
                traits_t::construct(alloc, new_data + i, std::move(data_[i]));
                traits_t::destroy(alloc, data_ + i);
            }

            for (size_type i = new_cap; i < size_; ++i) {
                traits_t::destroy(alloc, data_ + i);
            }

            size_ = new_cap;
        } else { 
            for (size_type i = 0; i < size_; ++i) {
                traits_t::construct(alloc, new_data + i, std::move(data_[i]));
                traits_t::destroy(alloc, data_ + i);
            }
        }
        
        if (data_ != nullptr) {
            traits_t::deallocate(alloc, data_, capacity_);
        }
        
        capacity_ = new_cap;
        data_ = new_data;
    }

    void Expand() {
        if (capacity_ == 0) {
            ResizeArray(16);
        } else {
            ResizeArray(capacity_ * 2);
        }
    }

    bool ShouldBeExpanded() {
        return capacity_ == size_;
    }

    void ExpandIfNeed() {
        if (ShouldBeExpanded()) {
            Expand();
        }
    }
};

// TODO Non-member functions