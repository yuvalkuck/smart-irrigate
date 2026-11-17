# utest/

A second host-native CMake project that fetches GoogleTest and fff (fake functions) and builds a
`tests` executable. It is not part of the firmware build:

```bash
cmake -S utest -B utest/build && cmake --build utest/build && ctest --test-dir utest/build
```
