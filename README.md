# Flag.hpp
A modern, zero-dependency, allocation-light command-line parsing library for C++23, inspired by the ergonomics of Go's `flag` package.
By aggressively utilizing `std::string_view` and `std::variant`, **Flag.hpp** guarantees high performance with virtually zero runtime heap allocations during the parsing loop.

## Features

* **Header-Only:** Drop a single file into your project and compile.
* **Go-Inspired API:** No complex builder patterns. Declare standard C++ variables and bind them directly to the parser.
* **Type-Safe:** Fully leverages C++23 features to ensure safe, predictable type casting.
* **Tsoding-Style Comments:** Safely ignore specific flags during development without deleting them from your run configurations.

The following forms are permitted:
```bash
    -flag
    --flag   // double dashes are also permitted
    -flag=x
    -flag x  // non-boolean flags only
    -/flag   // @Tsoding style commented flags
    --/flag
```
> **Note on empty strings:** ` ./prog -flag=` or `./prog -flag=""` are not currently supported. To pass an empty string, explicitly use the space-separated format: `./prog -flag ""`

## Installation
Just copy the Flag.hpp into your source code and you are good to go.

## Usage
First create an object of flag::CmdParser Class, then just use register_variable method on that object to declare flags.
After the declaration of flags just call object.parse with argc and argv as params.
The variables will be populated based on the flags in parse method and any additional positional args can obtained by the get_pos_args method on that object.
Refer to the Example.cpp for an example.
