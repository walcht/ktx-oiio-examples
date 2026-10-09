# About

Samples on how to use OpenImageIO KTX2 plugin to read/write multiple texture
formats.

## Building

```
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=address,undefined
cmake --build build/ -j8
```

## License

MIT License. See LICENSE.txt.
