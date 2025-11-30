
#ifndef MIZ_ITERATORS_HPP
#define MIZ_ITERATORS_HPP

#include <stdexcept>
#include <vector>
#include <iostream>

template <typename T>
struct iterator_viewer {
    private :
    T* beg;
    T* arr_end;
    T* iter;

    public : 

    void transport(const iterator_viewer<T>& oth) {
        beg = oth.beg;
        iter = oth.iter;
        arr_end = oth.arr_end;
    }

    void assign(T* arr, size_t arr_size){
        beg = arr;
        iter = arr;
        arr_end = &arr[arr_size];
    }

    iterator_viewer() = default;
    iterator_viewer(const iterator_viewer<T>& oth) {
        if (this != &oth) {
            transport(oth);
        }
    }

    iterator_viewer(iterator_viewer<T>&& oth){
        transport(oth);
        oth.cancel();
    }

    iterator_viewer(std::vector<T>& vec) { assign(vec); }

    template <size_t arr_size> 
    iterator_viewer(const std::array<T, arr_size>& arr) { assign(arr); }

    iterator_viewer<T>& operator=(const iterator_viewer<T>& oth){
        if (this != &oth) {
            transport(oth);
        }
        return *this;
    }

    iterator_viewer<T>& operator=(iterator_viewer<T>&& oth){
        transport(oth);
        oth.cancel();
        return *this;
    }

    iterator_viewer<T>& operator=(std::vector<T>& vec) {
        assign(vec);
        return *this;
    }

    template <size_t arr_size>
    iterator_viewer<T>& operator=(std::array<T, arr_size>& arr) {
        assign<arr_size>(arr);
        return *this;
    }

    template <size_t arr_size>
    void assign(const std::array<T, arr_size>& arr){ assign(arr.data(), arr_size); }
    void assign(std::vector<T>& vec) { assign(vec.data(), vec.size()); }

    const T* get_val() const noexcept { return iter; }
    T value() const noexcept { return *iter; }
    void operator++() noexcept { iter++; }
    bool end_reached() const noexcept { return (arr_end == iter); }
    void operator--() noexcept { iter--; }
    void rewind() noexcept { iter = beg; }
    size_t count_iterated() const noexcept { return (iter - beg); }
    size_t count_iterable() const noexcept { return (arr_end - iter); }
    size_t total_length() const noexcept { return (arr_end - beg); }

    void cancel() noexcept {
        beg = nullptr;
        arr_end = nullptr;
        iter = nullptr;
    }

    class iterator {
        private :
        T* iter;

        public :

        iterator(T* ptr) : iter(ptr) {}

        iterator& operator++() noexcept {
            ++iter;
            return *this;
        }

        iterator operator++(int) noexcept {
            iterator temp(iter);
            ++iter;
            return temp;
        }

        bool operator==(const iterator& oth) const noexcept { return (iter == oth.iter); }
        bool operator!=(const iterator& oth) const noexcept { return !(*this == oth); }

        iterator& operator--() noexcept {
            --iter;
            return *this;
        }

        iterator operator--(int) noexcept {
            iterator temp(iter);
            --iter;
            return temp;
        }
        
        const T& operator*() { return *iter; }
        const T* operator->() { return iter; }
    };

    iterator begin() noexcept { return iterator(beg); }
    iterator end() const noexcept { return iterator(arr_end); }
};


template <typename T>
// Lightweight iterator-based array holder (now safer)
struct iterator_array
{   
    private :
    T* beg = nullptr;
    T* end_added = nullptr;
    T* end_allocated = nullptr;

    public:

    // Default constructor
    iterator_array() = default;
    
    void assign(T* arr, size_t size) noexcept
    {   
        beg = arr;
        end_added = arr;
        end_allocated = arr + size;
    }

    void assign(T* arr, size_t size, size_t many_were_added)
    {
        if(many_were_added > size) throw std::invalid_argument("Can't assign array, many_were_added was out of range");
        beg = arr;
        end_added = &arr[many_were_added];
        end_allocated = arr + size;        
    }

    // Assign method - Initializes the array pointers
    template <size_t arr_size>
    void assign(std::array<T, arr_size>& arr) noexcept { assign(arr.data(), arr_size); }

    // Assign method - Initializes with elements already added
    template <size_t arr_size>
    void assign(std::array<T, arr_size>& arr, size_t many_were_added) { assign(arr.data(), arr_size, many_were_added); }

    iterator_array<T>& operator=(const iterator_array<T>& oth) 
    {   
        if(this != &oth){
            beg = oth.beg;
            end_added = oth.end_added;
            end_allocated = oth.end_allocated;
        }
        return *this;
    }

    void operator=(iterator_array<T>&& oth)
    {     
        beg = oth.beg;
        end_added = oth.end_added;
        end_allocated = oth.end_allocated;
        oth.cancel();
    }

    T& operator[](size_t index) 
    {
        return beg[index];
    }

    // Utility methods for the Parser to use
    bool full() const { return end_added == end_allocated; }
    size_t capacity() const { return end_allocated - beg; }
    size_t size() const { return end_added - beg; }
    void push_back(const T& item)
    {
        *end_added = item;
        end_added++;
    }

    void push_back(T&& item)
    {
        
        *end_added = std::move(item);
        end_added++;
        
    }

    void pull_back()
    {
        if(end_added == beg) return;

        end_added--;
        end_added->~T();
    }

    const T* begin_arr() const noexcept { return beg; }
    const T* end_arr() const noexcept { return end_allocated; }
    const T* end_filled() const noexcept { return end_added; }

    iterator_viewer<T> get_viewer() const {
        iterator_viewer<T> viewer;
        viewer.assign(beg, (end_added - beg));
        return viewer;
    }

    void cancel() noexcept {
        end_allocated = nullptr;
        beg = nullptr;
        end_added = nullptr;
    }

    class iterator {
        private :
        T* iter;

        public :

        iterator(T* ptr) : iter(ptr) {}

        iterator& operator++() noexcept {
            ++iter;
            return *this;
        }

        iterator operator++(int) noexcept {
            iterator temp(iter);
            ++iter;
            return temp;
        }

        bool operator==(const iterator& oth) const noexcept { return (iter == oth.iter); }
        bool operator!=(const iterator& oth) const noexcept { return !(*this == oth); }

        iterator& operator--() noexcept {
            --iter;
            return *this;
        }

        iterator operator--(int) noexcept {
            iterator temp(iter);
            --iter;
            return temp;
        }
        
        T& operator*() { return *iter; }
        T* operator->() { return iter; }
    };

    iterator begin() noexcept { return iterator(beg); }
    iterator end() const noexcept { return iterator(end_added); }
};

#endif