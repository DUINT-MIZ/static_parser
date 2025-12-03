#ifndef MIZ_STATIC_PARSER_HPP
#define MIZ_STATIC_PARSER_HPP

/*
Welcome welcome everybody
To graaandd finale
we're bout to set some optimization here
our premature optimization has the stage !
*/


#include <type_traits>
#include <array>
#include <vector>
#include <stdexcept>
#include <functional>
#include <cstring>
#include <iostream>
#include <frozen/unordered_map.h>
#include <frozen/string.h>

namespace fragile {


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

    constexpr iterator_viewer(T* arr, size_t size) noexcept {
        beg = arr;
        iter = arr;
        arr_end = arr + size;
    }

    template <typename U = T, typename std::enable_if<!std::is_const<U>::value, void>::type>
    constexpr iterator_viewer(std::vector<T>& vec) noexcept {
        beg = vec.data();
        iter = vec.data();
        arr_end = beg + vec.size();
    }

    template <size_t N>
    constexpr iterator_viewer(const std::array<T, N>& arr) noexcept {
        beg = arr.data();
        iter = arr.data();
        arr_end = beg + N;
    }

    constexpr iterator_viewer& operator=(const iterator_viewer<T>&oth) noexcept {
        if(&oth != this) transport(oth);
        return *this;
    }

    constexpr iterator_viewer& operator=(iterator_viewer<T>&& oth) noexcept { 
        if(&oth != this) transport(oth); 
        oth.cancel();
    }

    template <typename U = T, typename std::enable_if<!std::is_const<U>::value, void>::type>
    constexpr iterator_viewer& operator=(std::vector<T>& vec) noexcept {
        beg = vec.data();
        iter = vec.data();
        arr_end = beg + vec.size();
        return *this;
    }

    template <size_t N>
    constexpr iterator_viewer& operator=(std::array<T, N>& arr) noexcept {
        beg = arr.data();
        iter = arr.data();
        arr_end = beg + N;
        return *this;
    }

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

    constexpr inline void rewind() noexcept { iter = beg; }
    constexpr size_t count_iterated() const noexcept { return iter - beg; }
    constexpr size_t count_iterable() const noexcept { return arr_end - iter; }
    constexpr size_t total_size() const noexcept { return arr_end - beg; }
    constexpr bool end_reached() const noexcept { return (iter == arr_end); }

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
    static_assert(!std::is_const<T>::value ||
        std::is_pointer<T>::value,
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

    constexpr inline void assign(const T* data, size_t size, size_t offset) noexcept {
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

    T& operator[](size_t index) noexcept { return beg[index]; }

    constexpr iterator_viewer<T> get_viewer() const noexcept {
        return iterator_viewer<T>(beg, (end_added - beg));
    }
    
    constexpr size_t count_insertable() const noexcept { return (end_allocated - end_added); }
    constexpr size_t count_inserted() const noexcept { return (end_added - beg); }
    constexpr size_t total_capacity() const noexcept { return (end_allocated - beg); }
    constexpr bool full() const noexcept { return (end_added == end_allocated); }
    constexpr bool empty() const noexcept { return (end_added == beg); }

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

class storage_full : public std::runtime_error {
public:
    storage_full(const std::string& msg)
        : std::runtime_error(msg) {}
};

class prof_restriction : public std::runtime_error {
public:
    prof_restriction(const std::string& msg)
        : std::runtime_error(msg) {}
};

class token_error : public std::runtime_error {
public:
    token_error(const std::string& msg)
        : std::runtime_error(msg) {}
};

constexpr std::array<bool, 256> make_table(){
    std::array<bool, 256> data {};

    for(int i = 0; i < 256; i++){ data[i] = false; }

    for(unsigned char c = '0'; c <= '9'; c++){
        data[c] = true;
    }

    for(unsigned char c = 'A'; c <= 'Z'; c++){
        data[c] = true;
    }

    for(unsigned char c = 'a'; c <= 'z'; c++){
        data[c] = true;
    }

    data['_'] = true;
    return data;
}

constexpr auto char_table = make_table();


constexpr bool valid_long_opt_name(const char* name)
{   
    if(name == nullptr) return false;
    if(std::strlen(name) < 3) return false;
    if((name[0] != '-') || (name[1] != '-')) return false;
    name += 2;
    while(name[0] != '\0') { 
        if(!char_table[name[0]]) return false;
        name++;
    }
    return true;
}

constexpr bool valid_short_opt_name(const char* name)
{
    if(name == nullptr) return false;
    if(std::strlen(name) != 2) return false;
    return ((name[0] == '-') && ((name[1] == '_') ? false : char_table[name[1]]));
}

constexpr bool valid_posarg_name(const char* name)
{
    if(name == nullptr) return false;
    while(name[0] != '\0') {
        if(!char_table[name[0]]) return false;
        name++;
    }
    return true;
}

namespace behave {
    constexpr uint IS_STRICT = 1 << 2;
    constexpr uint IS_REQUIRED = 1 << 1;
    constexpr uint IS_IMMEDIATE = 1 << 0;
}

struct static_profile_context;

template <size_t identifier_count, size_t posarg_count>
class static_parser;

struct static_profile {
    const char* lname = nullptr;
    char sname = '\0';
    size_t narg = 0;
    bool is_immediate = 0;
    bool is_required = 0;
    bool is_strict = 1;
    int permitted_call_count = 1;
    static_profile_context* context;
    unsigned long conflict_mask = 0;

    constexpr static_profile() = delete;
    constexpr static_profile(static_profile_context& new_context) noexcept : context(&new_context) {}

    constexpr static_profile& identifier(const char* new_lname, char new_sname) noexcept {
        lname = new_lname;
        sname = new_sname;
        return *this;
    }

    constexpr static_profile& behavior(uint flag) noexcept {
        is_immediate = (flag & behave::IS_IMMEDIATE);
        is_required = (flag & behave::IS_REQUIRED);
        is_strict = (flag & behave::IS_STRICT);
        return *this;
    }

    constexpr static_profile& set_narg(size_t new_narg) noexcept {
        narg = new_narg;
        return *this;
    }

    constexpr static_profile& call_count(int new_call_count) noexcept {
        permitted_call_count = new_call_count;
        return *this;
    }
};

struct static_profile_context {
    private :
    bool is_called = false;
    uint count_called = 0;
    template <size_t identifier_count, size_t posarg_count> friend class static_parser;
    
    public :
    
    iterator_array<const char*> values;
    std::function<void(static_profile)> func = [](static_profile _){};

    static_profile_context() = delete;
    template <typename... Args>
    static_profile_context(Args&&... iterator_array_constructor_arg) 
        : values(std::forward<Args>(iterator_array_constructor_arg)...) {}

    bool called() const noexcept { return is_called; }
    uint call_count() const noexcept { return count_called; }
};

template <size_t identifier_count, size_t posarg_count>
class static_parser {
private:
    const frozen::unordered_map<frozen::string, const fragile::static_profile*, identifier_count>* map;
    
    const fragile::static_profile* posarg_data[posarg_count] {};
    iterator_array<const fragile::static_profile*> posargs;

    iterator_viewer<const fragile::static_profile> profile_ref;

    size_t posarg_size = 0;

    constexpr void validate_connection(
        const char* name, 
        const fragile::static_profile* target_prof
    ) const {
        auto it = map->find(frozen::string(name));

        if(it == map->end()) {    
            throw "static_parser : Such a name doesn't exist";
        }

        if(it->second != target_prof) {
            throw "static_parser Error : Duplicate profile identifier";
        }
    }

    inline void finalize() const {
        for(auto& prof : profile_ref) {
            if(prof.is_required && !prof.context->is_called) {
                std::cerr << "You didn't call for \'";
                if(prof.lname) std::cerr << prof.lname << "\'";
                else std::cerr << prof.sname << "\'";
                std::cerr << " it's a required argument";
                throw prof_restriction("uncalled_required");
            }
        }

        for(auto& prof : profile_ref) {
            if(prof.context->is_called) prof.context->func(prof);
        }
    }

    void fetch(
        const fragile::static_profile& prof,
        iterator_viewer<const char*>& input_viewer,
        bool (*check)(const char*) = [](const char*_){ return false; }
    ) const {
        
        size_t narg = prof.narg;
        auto& value = prof.context->values;
        const char* token = "";
        
        while(!input_viewer.end_reached()) {
            if(value.full()) break;
            if((narg == 0) && prof.is_strict) break;

            token = *input_viewer;
            if(check(token)) break;
            
            value.push_back(token);
            ++input_viewer;
            --narg;
        }
        
        if((signed)narg > 0) {
            std::cerr << "Unfinished narg (";
            if(prof.lname != nullptr) {
                std::cerr << prof.lname;
            } else {
                std::cerr << prof.sname;
            }
            std::cerr << ")\n\tNeeded : " << prof.narg << " Args, got " << (prof.narg - narg) << std::endl;
            throw prof_restriction("unfinished_narg");
        }

        prof.context->is_called = true;
    }

    // Return how many required option it parsed
    inline char parse_opt(
        iterator_viewer<const char*>& argv_iter,
        iterator_array<const char*>& dump
    ) const {
        auto prof_it = map->end();
        auto end = map->end();
        const char* token = "";
        const fragile::static_profile* prof_ptr = nullptr;

        unsigned long conflict_status = 0;        

        while(!argv_iter.end_reached()){
            token = *argv_iter;
            
            if(token[0] == '-') {
                if(std::strcmp(token , "--") == 0) break;
                
                prof_it = map->find(frozen::string(token));
                if(prof_it == end) {
                    std::cerr << "Unknown Flag : \"" << token << "\" is an unknown flag" << std::endl;
                    throw token_error("unknown_token");
                }
                prof_ptr = prof_it->second;

                if(conflict_status & prof_ptr->conflict_mask) {
                    std::cerr << "Conflict occurred for flag " << token << std::endl;
                    throw prof_restriction("conflict");
                }
                conflict_status |= prof_ptr->conflict_mask;

                ++argv_iter;

                fetch(*prof_ptr, argv_iter, [](const char* token){ return (token[0] == '-'); });
                if(prof_ptr->is_immediate) {
                    prof_ptr->context->func(*prof_it->second);
                    return 'h';
                }
            } else {
                if(dump.full()) {
                    std::cerr << "Dump Overflow : You threw too much argument for posargs" << std::endl;
                    throw storage_full("full");
                }
                
                dump.push_back(token);
                ++argv_iter;
            }
        }
        return 's';
    }

    inline void parse_posarg(iterator_array<const char*>& dump) const {
        auto dump_viewer = dump.get_viewer();
        auto posarg_viewer = posargs.get_viewer();
        while(!posarg_viewer.end_reached()) {
            fetch(**posarg_viewer, dump_viewer);
            ++posarg_viewer;
        }
        if(!dump_viewer.end_reached()) {
            std::cerr << "Unknown Token : unknown series of token starting from " << *dump_viewer << std::endl;
            throw token_error("unknown_token");
        }
        if(!posarg_viewer.end_reached()) {
            std::cerr << "Uncalled Posargs : You didn't call all of the posarg, there were still " << posarg_viewer.count_iterable() << " Posarg left" << std::endl;
            throw prof_restriction("uncalled_required");
        }
    }
    public:
    static_parser() = delete;

    template <typename... Args>
    constexpr static_parser(
        const frozen::unordered_map<frozen::string, const fragile::static_profile*, identifier_count>& new_map,
        Args&&... option_iterator_array_constructor_arg
    ) : map(&new_map),
        profile_ref(std::forward<Args>(option_iterator_array_constructor_arg)...){
            posargs.assign(posarg_data, posarg_count); 
        size_t valid_mappings = 0;
        char sname_buf[3] = {'-', '-', '\0'};
        for(const fragile::static_profile& prof : profile_ref) {
            if(prof.lname != nullptr) {
                validate_connection(prof.lname, &prof);
                valid_mappings++;
                if(prof.lname[0] != '-') {
                    posargs.push_back(&prof);
                    if(prof.sname != '\0') throw "static_parser Error : Positional argument may not have a short name identifier";
                }
            }

            if(prof.sname != '\0') {
                sname_buf[1] = prof.sname;
                validate_connection(sname_buf, &prof);
                valid_mappings++;
            }
        }
        
        if(valid_mappings != map->size()) {
            throw "static_parser Error: Map size does not match total flags in profiles (Unmapped keys exist?)";
        }
    }

    template <size_t dump_size>
    inline void parse(const char** argv, int argc) const {
        const char* dump_data[dump_size];
        iterator_viewer<const char*> argv_iter(argv, argc);
        iterator_array<const char*> dump(dump_data, dump_size);

        if(parse_opt(argv_iter, dump) == 'h') return;
        parse_posarg(dump);
        finalize();
    }
};

}

#endif