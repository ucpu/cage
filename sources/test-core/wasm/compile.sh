#!/bin/bash
PATH="$PATH:/c/Program Files/LLVM/bin"
clang --target=wasm32 -Wl,--export-all -Wl,--no-entry -nostdlib -o sums.wasm sums.c
clang --target=wasm32 -Wl,--allow-undefined -Wl,--export-all -Wl,--no-entry -nostdlib -o strings.wasm strings.c
clang --target=wasm32 -Wl,--allow-undefined -Wl,--export-all -Wl,--no-entry -nostdlib -o natives.wasm natives.c
