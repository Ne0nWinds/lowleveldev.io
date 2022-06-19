"use strict";

const { instance } = await WebAssembly.instantiateStreaming(
	fetch("./build/binary.wasm")
);

export default instance.exports;
