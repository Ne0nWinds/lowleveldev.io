
if (!(Test-Path -Path build)) { mkdir build }
pushd
cd build

clang -g -O0 -D_DEBUG --target=wasm32 -msimd128 -mbulk-memory -nostdlib `
"-Wl,--no-entry,--reproduce=binary.wasm.map" `
-Wno-incompatible-library-redeclaration -Wno-switch `
-o binary.wasm `
..\src\main.c ..\src\memory.c ..\src\tokenizer.c

popd
