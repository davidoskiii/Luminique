# Luminique Programming Language

Luminique is a high-level programming language designed to provide developers with a simple and elegant syntax reminiscent of JavaScript, while offering the performance benefits of a bytecode language. It is implemented in C, making it efficient and versatile for a wide range of applications.

## Features

- **High-level syntax**: Luminique's syntax is designed to be intuitive and easy to understand, making it accessible to both novice and experienced programmers.
- **Bytecode execution**: Luminique programs are compiled to bytecode, which is then executed by the Luminique virtual machine (LVM). This allows for efficient execution and portability across different platforms.
- **Garbage collection**: Luminique features automatic memory management through a garbage collector, relieving developers from manual memory management concerns.
- **Interoperability**: Luminique can easily interface with existing C code, allowing developers to leverage existing libraries and tools.
- **Dynamic typing**: Luminique supports dynamic typing, enabling flexible and expressive code without the need for explicit type declarations.
- **Standard library**: Luminique comes with a standard library that provides commonly used functionality, such as file I/O, networking, and data manipulation.

## Installation

To install Luminique, follow these steps:

1. Clone the Luminique repository from GitHub:

```
git clone https://github.com/davidoskiii/Luminique.git
```

2. Compile it (working on adding a makefile):
```
gcc main.c chunk/chunk.c debug/debug.c memory/memory.c value/value.c vm/vm.c compiler/compiler.c scanner/scanner.c object/object.c table/table.c native/n
ative.c std/std.c assert/assert.c arrays/arrays.c -o luminique -lm
```

3. Try it out with the examples in the tests directory.
