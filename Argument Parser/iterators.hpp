#include <stdexcept>
#include <vector>

template <typename T>
struct iterator_viewer {
    private :
    T* beg;
    T* end;
    T* iter;

    public : 

    void assign(T* arr, size_t arr_size){
        beg = arr;
        iter = arr;
        end = &arr[arr_size];
    }

    iterator_viewer() = default;
    iterator_viewer(const iterator_viewer<T>& oth) {
        if (this != &oth) {
            beg  = oth.beg;
            iter = oth.iter;
            end  = oth.end;
        }
    }

    iterator_viewer(iterator_viewer<T>&& oth){
        beg = oth.beg;
        iter = oth.iter;
        end = oth.end;
        oth.cancel();
    }

    iterator_viewer<T>& operator=(const iterator_viewer<T>& oth){
        if (this != &oth) {
            beg  = oth.beg;
            iter = oth.iter;
            end  = oth.end;
        }
        return *this;
    }

    iterator_viewer<T>& operator=(iterator_viewer<T>&& oth){
        beg = oth.beg;
        iter = oth.iter;
        end = oth.end;
        oth.cancel();
    }

    template <size_t arr_size>
    void assign(std::array<T, arr_size>& arr){ assign(arr.data(), arr_size); }
    void assign(std::vector<T>& vec) { assign(vec.data(), vec.size()); }

    const T* get_val() const noexcept { return iter; }
    T value() const noexcept { return *iter; }
    void operator++() noexcept { iter++; }
    bool end_reached() const noexcept { return (end == iter); }
    void operator--() noexcept { iter--; }
    void rewind() noexcept { iter = beg; }
    size_t count_iterated() const noexcept { return (iter - beg); }
    size_t count_iterable() const noexcept { return (end - iter); }
    size_t total_length() const noexcept { return (end - beg); }

    void cancel() noexcept {
        beg = nullptr;
        end = nullptr;
        iter = nullptr;
    }

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

    const T* begin() const noexcept { return beg; }
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
};