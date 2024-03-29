Param (
	[switch]$wat
)

if (!(Test-Path -Path build)) { mkdir build }
pushd
cd build

if ($wat) {

clang -g -O0 -D_DEBUG --target=wasm32 -msimd128 -mbulk-memory -nostdlib `
"-Wl,--no-entry,--reproduce=binary.wasm.map" `
-Wno-incompatible-library-redeclaration -Wno-switch `
-o binary.wasm `
../src/main.c ../src/memory.c ../src/tokenizer.c ../src/parser.c ../src/code_gen.c ../src/code_gen_wat.c

} else {

clang -g -O0 -D_DEBUG --target=wasm32 -msimd128 -mbulk-memory -nostdlib `
"-Wl,--no-entry,--reproduce=binary.wasm.map" `
-Wno-incompatible-library-redeclaration -Wno-switch `
-o binary.wasm `
../src/main.c ../src/memory.c ../src/tokenizer.c ../src/parser.c ../src/code_gen.c ../src/code_gen_wasm.c

}

popd
