# SPL: Standard Parser Library

Hobby project of mine

**SPL** is a lightweight, flexible, and dependency-free C++ argument parsing library. It is designed to be efficient and easy to integrate, relying solely on the C++ Standard Template Library (STL).

The library operates on a **Dual-Mode** philosophy:

1.  **Dynamic Parser (`Dy_parser`):** User-friendly, automatic memory management (heap), suitable for standard desktop/server applications.
2.  **Stack Parser (`St_parser`):** **Zero-allocation** focused, manual memory management via external stack/buffers, highly optimized for **embedded systems** or performance-critical applications.

## Key Features

  * **Header-only:** No complex build systems required, simply include the headers.
  * **Dependency-Free:** Pure C++, depends only on STL.
  * **Positional Arguments:** Handles arguments without flags (e.g., `cp source dest`).
  * **Flag Combinations:** Supports short flags (`-f`), long flags (`--force`), or both.
  * **Immediate Callbacks:** Execute functions immediately upon flag detection (perfect for `--help` or `--version`).
  * **Strict Parsing:** Full control over how many arguments a flag consumes.
  * **Safe Iterators:** Uses custom `iterator_viewer` and `iterator_array` for safe and fast memory navigation.

-----

## 📦 Installation

Since SPL is **header-only**, you only need to copy the header files into your project directory.

Recommended directory structure:

```text
project/
├── main.cpp
└── include/
    ├── spl/
    │   ├── iterators.hpp
    │   ├── spl.hpp
    │   ├── dynamic_parser.hpp
    │   └── stack_parser.hpp
```

Ensure your compiler supports at least **C++17** (required for `std::string_view` and other modern features).

```bash
g++ main.cpp -o app -std=c++17 -I./include
```

-----

## 💡 Core Concepts

Before using the library, familiarize yourself with these terms:

  * **Profile:** The definition of an argument (name, type, required status, etc.).
  * **Viewer (`iterator_viewer`):** A lightweight wrapper (smart pointer-like) to view data without copying it.
  * **Long/Short Option:**
      * Short: Single character with a single dash (e.g., `-v`).
      * Long: String with double dashes (e.g., `--verbose`).
  * **PosArg (Positional Argument):** Arguments without a prefix dash where order matters.

-----

## ⚡ Quick Start: Dynamic Parser

Use `Dy_parser` for general-purpose applications where dynamic memory allocation (heap) is acceptable. This is the easiest way to get started.

### Example (`main.cpp`)

```cpp
#include <iostream>
#include "dynamic_parser.hpp" // Ensure the path is correct

int main(int argc, char* argv[]) {
    try {
        // 1. Initialize Parser
        Dy_parser parser;

        // 2. Add Option: --name or -n, requires 1 argument, mandatory
        parser.add_opt(LongShort, true, 1, "--name", "-n")
              .set_callback([](dy_profile& p){
                  // Callback lambda when parsing completes for this option
                  std::cout << "Hello, " << p.value_viewer().value() << "!\n";
              });

        // 3. Add Flag: --verbose or -v, takes 0 arguments
        parser.add_opt(LongShort, false, 0, "--verbose", "-v")
              .set_callback([](dy_profile& p){
                  std::cout << "[Verbose Mode Active]\n";
              });

        // 4. Add Positional Argument (e.g., input_file)
        parser.add_posarg(1, "input_file")
              .set_callback([](dy_profile& p){
                  std::cout << "Processing file: " << p.value_viewer().value() << "\n";
              });

        // 5. Execute Parsing
        // Convert argc/argv into a vector for the library
        std::vector<const char*> args(argv + 1, argv + argc);
        parser.parse(args);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

**Running the app:**

```bash
./app --name User -v data.txt
```

-----

## 🛠 Advanced: Stack Parser (Embedded Mode)

Use `St_parser` if you are working in a constrained environment or want to avoid heap allocation entirely.

**Note:** In this mode, the **User** is responsible for providing the buffers (storage) to hold the profiles and argument values.

### How to use `St_parser`

1.  Prepare a buffer (`std::array`) to store Profiles (Option definitions).
2.  Prepare a buffer to store the *values* of the parsed arguments.
3.  Pass these buffers into the parser.

### Example Code

```cpp
#include <iostream>
#include "stack_parser.hpp"

int main(int argc, char* argv[]) {
    // A. Prepare Storage for Profiles (Option Definitions)
    // Capacity: Max 5 Options and 2 Positional Arguments
    std::array<st_profile, 5> opt_storage;
    std::array<st_profile, 2> pos_storage;

    // B. Initialize Parser with the storage
    St_parser parser(opt_storage, pos_storage);

    // C. Prepare Value Buffer
    // This buffer will store pointers to the input strings
    static std::array<const char*, 1> name_val_buff; 

    try {
        // D. Register Option with specific value buffer
        parser.add_opt(
            name_val_buff,   // Buffer to store this argument's value
            LongShort,       // Type (Long & Short)
            true,            // Required?
            1,               // Number of arguments (narg)
            "--output",      // Long Name
            "-o"             // Short Name
        ).set_callback([](st_profile& p){
            std::cout << "Output set to: " << p.value_viewer().value() << "\n";
        });

        // E. Parse (using std::vector/array for input)
        std::vector<const char*> args(argv + 1, argv + argc);
        parser.parse(args);

    } catch (const std::exception& e) {
        std::cerr << "Parsing Error: " << e.what() << "\n";
    }

    return 0;
}
```

-----

## ⚙️ Option Configuration

Both `st_profile` and `dy_profile` share the same chaining methods for advanced configuration:

### 1\. `set_strict(bool)`

Enables strict mode. If `true`, the parser stops consuming arguments for this option immediately after `narg` is satisfied.

```cpp
opt.set_strict(true);
```

### 2\. `set_imme(bool)` (Immediate)

If `true`, the callback is triggered **immediately** upon finding the token, and the program exits (`exit(0)`) afterward. Highly useful for `--help` or `--version`.

```cpp
parser.add_opt(Long, false, 0, "--help", nullptr)
      .set_imme(true)
      .set_callback([](auto& p){
          print_help();
          // Library will automatically call exit(0)
      });
```

### 3\. `call_count(int)`

Limits how many times an option can be called.

  * `1`: Can appear only once (Default).
  * `N`: Can appear N times.
  * `-1` (or \< 0): Unlimited (Can appear multiple times).

<!-- end list -->

```cpp
// Example: -v -v -v (verbosity level)
parser.add_opt(Short, false, 0, nullptr, "-v")
      .call_count(3); // Max 3 times
```

-----

## 📚 API Reference

### `enum add_type`

Used in `add_opt` to define the flag type.

  * `Short` (1): Short flag only (e.g., `-f`).
  * `Long` (2): Long flag only (e.g., `--file`).
  * `LongShort` (3): Both.

### Methods `Dy_parser` & `St_parser`

#### `add_opt(...)`

Adds an option argument.

  * **Returns:** Reference to profile (`dy_profile&` or `st_profile&`).
  * **Params:** Flag Type, Required (bool), Nargs (int), Long Name, Short Name.
  * *(St\_parser only)* The first parameter is the `std::array` buffer to store values.

#### `add_posarg(...)`

Adds a positional argument (no flag).

  * **Params:** Nargs (int), Name (string identifier).
  * *(St\_parser only)* The first parameter is the `std::array` buffer.

#### `parse(Args...)`

Initiates the parsing process. Accepts `std::vector`, `std::array`, or C-style array pointers compatible with `iterator_viewer`.

### Helper: `iterator_viewer<T>`

Utility class to access parsed data.

  * `value()`: Get current element value.
  * `get_val()`: Get pointer to element.
  * `operator++`: Move to next element.
  * `end_reached()`: Check if iteration is done.
  * `rewind()`: Reset to beginning.

-----

## ⚠️ Error Handling

The library throws exceptions inheriting from `std::runtime_error`. It is highly recommended to wrap `parser.parse()` in a `try-catch` block.

| Exception Class | Description |
| :--- | :--- |
| **`token_error`** | Unknown token or invalid flag format (e.g., `--` without name). |
| **`prof_restriction`** | Profile rule violation (e.g., missing required arg, insufficient args, or call count exceeded). |
| **`storage_full`** | *(St\_parser only)* The provided buffer is full/insufficient. |
| **`std::invalid_argument`** | Developer error during setup (e.g., illegal characters in option name). |

-----

### Example Error Messages

```text
Error: Permitted call count reaches 0, for : --output
Error: Invalid number of argument passed for "-i", Needed : 2
Error: A required option was not called
```