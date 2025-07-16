# CymCalc
A simple header only C library for symbolic computations. It is not ment to be feature full though I plan to improve over time. Main motivation behind the project is to replace Symbolic++ library with this in my other toy projects becouse I find Symbolic++ to be bothersome.

# How to run
Just include in your source file like this:
```c
#define CYMCALC_IMPLEMENTATION
#include "cymcalc.h"
```

since this depends on GMP you will need to include and link with it. For example you can build examples.c with gcc like this:
```sh
gcc examples.c -I <insert-path-to-GMP>\include -L<insert-path-to-GMP>\lib -lgmp -static
```