# Luminique Programming Language

Luminique is a high-level programming language designed to provide developers with a simple and elegant syntax reminiscent of JavaScript. It is implemented in C, making it efficient and versatile for a wide range of applications.

## Features

- **High-level syntax**: Luminique's syntax is designed to be intuitive and easy to understand, making it accessible to both novice and experienced programmers.
- **Bytecode execution**: Luminique programs are compiled to bytecode, which is then executed by the Luminique virtual machine (LVM). This allows for efficient execution and portability across different platforms.
- **Garbage collection**: Luminique features automatic memory management through a garbage collector, relieving developers from manual memory management concerns.
- **Dynamic typing**: Luminique supports dynamic typing, enabling flexible and expressive code without the need for explicit type declarations.
- **Object model**: In Luminique everything is an object and every object has a class, this includes `nil`, `true`, `false`, `int`, `float`, `string`, `function`, `class` etc.
- **Standard library**: Luminique comes with a standard library that is updated day by day.

## Installation

To install Luminique, follow these steps:

1. Clone the Luminique repository from GitHub:

```
git clone https://github.com/davidoskiii/Luminique.git
cd Luminique
```

2. Clear the build directory and compile the executable:
```
rm -rf ./build/*
cmake -S . -B ./build
cmake --build ./build
./build/luminique
```

3. Try it out with the examples in the tests directory or read the documentation (dosn't exist yet :D).
