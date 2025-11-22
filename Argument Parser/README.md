# Zero-Overhead Stack Parser

Yes I do use AI because I'm lazy making my own docs. plus it got some "inviting-message"\
\
A high-performance C++ command-line argument parser designed for **embedded systems** and performance-critical applications.

This library adheres to a **Zero-Heap Allocation** philosophy for its internal logic. It relies entirely on **User-Managed Memory**, where the user injects pre-allocated buffers (via `std::array`) that reside in the stack or static storage.

> **Warning:** This library is designed for **seasoned users**. You are fully responsible for the lifetime and validity of the containers you provide to the Parser.

## 🌟 Key Features

* **No Heap Allocation:** Utilizes external `std::array` for predictable memory usage.
* **Fluent Interface:** Configure options using intuitive method chaining (e.g., `.set_strict().set_callback(...)`).
* **Strict & Non-Strict Parsing:** Control whether an option must strictly satisfy its `narg` count or if it can yield to subsequent options.
* **Immediate Execution:** Support for flags like `--help` or `--version` that invoke a callback and immediately `exit(0)`.
* **Call Counting:** Limit how many times an option can be invoked.
* **Universal Reference Parsing:** The `parse` function accepts arguments via *perfect forwarding* for maximum flexibility.

## 📦 Installation

Ensure `iterators.hpp` is available in your include path. This library is **header-only**.

```cpp
#include "parser.hpp" // Adjust to your actual header filename
````

## 🚀 Quick Start

Here is a basic example of how to set up storage and parse arguments:

```cpp
#include <iostream>
#include <array>
#include "parser.hpp"

int main(int argc, char const *argv[]) {
    // 1. Prepare Storage 
    // (Static is recommended to prevent stack overflow with large arrays and ensure lifetime)
    static std::array<profile, 5> storage_options; // Max capacity: 5 options
    static std::array<profile, 2> storage_posargs; // Max capacity: 2 positional args

    // 2. Prepare Value Buffers for each option
    static std::array<const char*, 2> buff_opt_b; 

    // 3. Initialize Parser
    Parser parser(storage_options, storage_posargs);

    // 4. Register Options
    parser.add_opt<2>(buff_opt_b, Short, false, 1, nullptr, "-b")
          .set_callback([](){ std::cout << "Option -b triggered!\n"; })
          .set_strict(true);

    try {
        // 5. Execute Parsing (Skipping argv[0])
        parser.parse(++argv, --argc);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

## 📖 API Reference

### 1\. Parser Initialization

The `Parser` constructor requires two `std::array` references which serve as the "backing storage" for option profiles.

```cpp
template <size_t N_OPTS, size_t N_POS>
Parser(std::array<profile, N_OPTS>& opts, std::array<profile, N_POS>& posargs);
```

### 2\. Adding Options (`add_opt`)

```cpp
template <size_t storage_size>
profile& add_opt(
    std::array<const char*, storage_size>& arr, // Buffer to store parsed values
    add_type ins_type,                          // Short, Long, or LongShort
    bool is_required,                           // Is this option mandatory?
    size_t expected_narg,                       // Number of arguments expected
    const char* lname,                          // Long name (e.g., "--file"), or nullptr
    const char* sname                           // Short name (e.g., "-f"), or nullptr
);
```

### 3\. Profile Configuration (Fluent API)

The `profile` object returned by `add_opt` can be configured via method chaining:

| Method | Description | Default |
| :--- | :--- | :--- |
| `.set_strict(bool)` | If `true`, parsing for this option stops immediately if the next token looks like another option (starts with `-`), even if `narg` isn't satisfied. | `false` |
| `.set_imme(bool)` | If `true`, the `callback` is executed immediately upon detection, and the program calls `exit(0)`. Ideal for `--help`. | `false` |
| `.call_count(int)` | Maximum number of times this option can be called. Use `0` to disable, or `< 0` for infinite. | `1` |
| `.set_callback(func)` | Sets a `std::function<void()>` to be executed after parsing (or immediately if `is_immediate` is set). | - |

### 4\. Adding Positional Arguments (`add_posarg`)

Captures arguments that do not start with a hyphen (`-`).

```cpp
template <size_t storage_size>
profile& add_posarg(
    std::array<const char*, storage_size>& arr, 
    size_t narg_expected, 
    const char* name // Name used for error identification
);
```

### 5\. Execution (`parse`)

```cpp
template <typename... Args>
void parse(Args&&... args);
```

Accepts arguments via perfect forwarding. Typically takes `argv` pointer and `argc` count (or any iterator/size combination supported by the internal `iterator_viewer`).

-----

## ⚠️ Exception Handling

The Parser throws custom exceptions inheriting from `std::runtime_error`:

  * **`storage_full`**: Thrown when the main backing storage (`options` or `posargs`) is full. Increase the `std::array` size in `main`.
  * **`prof_restriction`**: Thrown when profile rules are violated:
      * Option called more than `permitted_call_count`.
      * Insufficient arguments (`narg`) provided.
      * Missing mandatory (`is_required`) option.
  * **`token_error`**: Thrown on unknown options or invalid token sequences.
  * **`std::overflow_error`**: If the number of arguments exceeds `MAX_ARG` (Default: 10).

## ⚙️ Compile-Time Configuration

  * **`MAX_ARG`**: Defined as 10 by default. You can modify `#define MAX_ARG` in the header if your application requires processing more command-line arguments.

-----

## 💡 Usage Tips

1.  **Use `static`:** When declaring the storage `std::array` inside functions, always use the `static` keyword. This prevents stack overflow on large arrays and ensures the memory remains valid throughout the parser's lifetime.
2.  **Strict Mode:** Use `.set_strict(true)` for flags or optional arguments to prevent the parser from "consuming" the next option as an argument for the current one.

<!-- end list -->

```
```