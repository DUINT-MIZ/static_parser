
#ifndef MIZ_ITERATORS_HPP
#define MIZ_ITERATORS_HPP

#include <stdexcept>
#include <array>
#include <vector>

template <typename T>
struct iterator_viewer {
    private :
    const T* beg = nullptr;
    const T* iter = nullptr;
    const T* arr_end = nullptr;

    
    inline constexpr void transport(const iterator_viewer<T>& oth) noexcept {
        beg = oth.beg;
        iter = oth.iter;
        arr_end = oth.arr_end;
    }
    
    public :
    
    inline constexpr void assign(const T* data, std::size_t size) {
        beg = data;
        iter = data;
        arr_end = data + size;
    }

    inline constexpr void cancel() {
        beg = nullptr;
        iter = nullptr;
        arr_end = nullptr;
    }

    constexpr iterator_viewer() = default;
    constexpr iterator_viewer(const iterator_viewer<T>&oth) { transport(oth); }
    constexpr iterator_viewer(iterator_viewer<T>&& oth) noexcept {
        transport(oth);
        oth.cancel();
    }

    constexpr iterator_viewer(const T* data, size_t size) noexcept {
        assign(data, size);
    }

    template <typename U = T, typename std::enable_if_t<!std::is_const_v<U>>>
    constexpr iterator_viewer(const std::vector<T>& vec) noexcept {
        assign(vec.data(), vec.size());
    }

    template <typename U = T, typename std::enable_if_t<!std::is_const_v<U>>>
    constexpr iterator_viewer(std::vector<T>&& vec) = delete;

    template <size_t N>
    constexpr iterator_viewer(const std::array<T, N>& arr) noexcept {
        assign(arr.data(), N);
    }

    template <size_t N>
    constexpr iterator_viewer(std::array<T, N>&& arr) = delete;

    constexpr iterator_viewer& operator=(const iterator_viewer<T>&oth) noexcept {
        if(&oth != this) transport(oth);
        return *this;
    }

    constexpr iterator_viewer& operator=(iterator_viewer<T>&& oth) noexcept { 
        if(&oth != this) transport(oth); 
        oth.cancel();
    }

    template <typename U = T, typename std::enable_if<!std::is_const<U>::value, void>::type>
    constexpr iterator_viewer& operator=(const std::vector<T>& vec) noexcept {
        assign(vec.data(), vec.size());
        return *this;
    }

    template <typename U = T, typename std::enable_if<!std::is_const<U>::value, void>::type>
    constexpr iterator_viewer& operator=(std::vector<T>&& vec) = delete;

    template <size_t N>
    constexpr iterator_viewer& operator=(const std::array<T, N>& arr) noexcept {
        assign(arr.data(), N);
        return *this;
    }

    template <size_t N>
    constexpr iterator_viewer& operator=(std::array<T, N>&& arr) = delete;

    constexpr const T& operator*() const noexcept { return *iter; }
    constexpr iterator_viewer& operator++() noexcept { 
        ++iter;
        return *this;
    }

    constexpr iterator_viewer operator++(int) noexcept {
        iterator_viewer temp(*this);
        ++iter;
        return temp;
    }

    constexpr iterator_viewer& operator--() noexcept {
        --iter;
        return *this;
    }

    constexpr iterator_viewer operator--(int) noexcept {
        iterator_viewer temp(*this);
        --iter;
        return temp;
    }

    constexpr iterator_viewer operator+(size_t offset) {
        iter += offset;
        iterator_viewer temp(*this);
        iter -= offset;
        return temp;
    }

    constexpr iterator_viewer& operator+=(size_t offset) {
        iter += offset;
        return *this;
    }

    constexpr iterator_viewer operator-(size_t offset) {
        iter -= offset;
        iterator_viewer temp(*this);
        iter += offset;
        return temp;
    }

    constexpr iterator_viewer& operator-=(size_t offset) {
        iter -= offset;
        return *this;
    }

    const T& operator[](std::size_t index) const noexcept { return beg[index]; }

    constexpr inline void rewind() noexcept { iter = beg; }
    constexpr std::size_t count_iterated() const noexcept { return iter - beg; }
    constexpr std::size_t count_iterable() const noexcept { return arr_end - iter; }
    constexpr std::size_t total_size() const noexcept { return arr_end - beg; }
    constexpr bool end_reached() const noexcept { return (iter == arr_end); }
    constexpr const T* front() const noexcept { return beg; }
    constexpr const T* now() const noexcept { return iter; }
    constexpr const T* back() const noexcept { return arr_end; }


    class iterator {
        private :
        const T* it;
        public :
        
        constexpr iterator() = delete;
        constexpr iterator(const T* iter) : it(iter) {}

        constexpr bool operator==(const iterator& oth) const noexcept { return (it == oth.it); }
        constexpr bool operator!=(const iterator& oth) const noexcept { return !(*this == oth); }

        constexpr iterator& operator++() noexcept {
            ++it;
            return *this;
        }

        constexpr iterator& operator++(int) noexcept {
            iterator temp(it);
            ++it;
            return temp;
        }

        constexpr iterator& operator--() noexcept {
            --it;
            return *this;
        }

        constexpr iterator& operator--(int) noexcept {
            iterator temp(it);
            --it;
            return temp;
        }

        constexpr const T& operator*() const noexcept { return *it; }
        constexpr const T* operator->() const noexcept { return it; }
    };

    constexpr iterator begin() const noexcept { return iterator(beg); }
    constexpr iterator end() const noexcept { return iterator(arr_end); }
};

template <typename T>
/*
Lightweight, 
*/
struct iterator_array {
    static_assert(!std::is_const_v<T> ||
        std::is_pointer_v<T>,
        "Really, what how do you think this can manage array data, if it's const ?");

    private :
    T* beg = nullptr;
    T* end_added = nullptr;
    T* end_allocated = nullptr;

    public :

    constexpr void transport(const iterator_array<T>& oth) noexcept {
        beg = oth.beg;
        end_added = oth.end_added;
        end_allocated = oth.end_allocated;
    }

    constexpr inline void assign(T* data, size_t size) noexcept {
        beg = data;
        end_added = data;
        end_allocated = beg + size;
    }

    constexpr inline void assign(T* data, size_t size, size_t offset) noexcept {
        beg = data;
        end_added = data + offset;
        end_allocated = beg + size;
    }

    void cancel(){
        beg = nullptr;
        end_added = nullptr;
        end_allocated = nullptr;
    }

    constexpr iterator_array() = default;
    constexpr iterator_array(const iterator_array<T>& oth) noexcept { transport(oth); }
    constexpr iterator_array(iterator_array<T>&& oth) noexcept {
        transport(oth);
        oth.cancel();
    }

    template <size_t N>
    constexpr iterator_array(std::array<T, N>& arr) noexcept { assign(arr.data(), N); }

    constexpr iterator_array(T* arr, size_t size) noexcept { assign(arr, size); }

    constexpr iterator_array<T>& operator=(const iterator_array<T>& oth) noexcept { 
        if(&oth != this) transport(oth);
        return *this;
    }

    constexpr iterator_array<T>& operator=(iterator_array<T>&& oth) noexcept {
        if(&oth != this) transport(oth);
        oth.cancel();
        return *this;
    }

    template <size_t N>
    constexpr iterator_array<T>& operator=(std::array<T, N>& arr) noexcept {assign(arr.data(), N); }
    
    constexpr void push_back(const T& item) {
        if(end_added >= end_allocated) throw std::overflow_error("iterator_array. can't push back. array full");
        *end_added = item;
        ++end_added;
    }

    constexpr void push_back(T&& item) {
        if(end_added >= end_allocated) throw std::overflow_error("iterator_array. can't push back. array full");
        *end_added = std::move(item);
        ++end_added;
    }

    constexpr void pull_back() {
        if(end_added <= beg) {
            end_added = beg;
            return;
        }
        --end_added;
        end_added->~T();
        
    }

    T& operator[](std::size_t index) noexcept { return beg[index]; }

    constexpr iterator_viewer<T> get_viewer() const noexcept {
        return iterator_viewer<T>(beg, (end_added - beg));
    }
    
    constexpr std::size_t count_insertable() const noexcept { return (end_allocated - end_added); }
    constexpr std::size_t count_inserted() const noexcept { return (end_added - beg); }
    constexpr std::size_t total_capacity() const noexcept { return (end_allocated - beg); }
    constexpr bool full() const noexcept { return (end_added == end_allocated); }
    constexpr bool empty() const noexcept { return (end_added == beg); }
    constexpr T& back() const noexcept { return *(end_added - 1); }
    constexpr T& front() const noexcept { return *beg; }

    class iterator {
        private :
        T* it;
        public :
        
        constexpr iterator() = delete;
        constexpr iterator(T* iter) : it(iter) {}

        constexpr bool operator==(const iterator& oth) const noexcept { return (it == oth.it); }
        constexpr bool operator!=(const iterator& oth) const noexcept { return !(*this == oth); }

        constexpr iterator& operator++() noexcept {
            ++it;
            return *this;
        }

        constexpr iterator& operator++(int) noexcept {
            iterator temp(it);
            ++it;
            return temp;
        }

        constexpr iterator& operator--() noexcept {
            --it;
            return *this;
        }

        constexpr iterator& operator--(int) noexcept {
            iterator temp(it);
            --it;
            return temp;
        }

        constexpr T& operator*() const noexcept { return *it; }
        constexpr T* operator->() const noexcept { return it; }
    };

    constexpr iterator begin() const noexcept { return iterator(beg); }
    constexpr iterator end() const noexcept { return iterator(end_added); }
};

#endif