import compiler from "./compiler.js"

const ENABLE_TEST_CASES = 1;

const editor = document.getElementById("editor");
const compile_button = document.getElementById("compile_button");
const wasm_exp_button = document.getElementById("wasm_explorer");

let program = null;
let binary = null;

let timeoutId = 0;
editor.oninput = async () => {
	program = await compile(editor.value);
}
// await editor.oninput();

compile_button.onclick = () => {
	const result = program.main();
	console.log(result);
}
window.onkeypress = (e) => {
	if (e.code == "Enter" && e.ctrlKey)
		compile_button.onclick();
}

let binaryExplorerEventListener = null;
wasm_explorer.onclick = async () => {
	if (binary && !binaryExplorerEventListener) {
		const dst = "https://wasdk.github.io/wasmcodeexplorer/?api=postmessage";
		const windowName = "BinaryExplorer";
		window.open(dst, "Binary Explorer", "popup");
		const binaryCopy = new Uint8Array(binary);
		binaryExplorerEventListener = (e) => {
			if (e.data.type == "wasmexplorer-ready") {
				window.removeEventListener("message", binaryExplorerEventListener, false);
				binaryExplorerEventListener = null;
				e.source.postMessage({
					type: "wasmexplorer-load",
					data: binaryCopy
				}, "*", [binaryCopy.buffer]);
			}
		};
		window.addEventListener("message", binaryExplorerEventListener, false);
	}
}

async function compile(value) {
	const encoder = new TextEncoder('utf-8');
	const view = new Uint8Array(compiler.memory.buffer, compiler.get_mem_addr(), 1024);

	const start = performance.now();
	encoder.encodeInto(value, view);
	view[value.length] = 0;

	const len = compiler.compile();
	if (len == 0) {
		console.log("== Compilation Failed == ");
		return;
	}

	binary = new Uint8Array(compiler.memory.buffer, compiler.get_compiled_code(), len);
	console.log(binary);
	const compilationToWebAssembly = performance.now();

	const { instance } = await WebAssembly.instantiate(binary);
	const webAssemblyToX86 = performance.now();
	console.log("Compilation to WebAssembly -- %.3fms", compilationToWebAssembly - start);
	console.log("WebAssembly to x86 -- %.3fms", webAssemblyToX86 - compilationToWebAssembly);
	console.log("Total Time -- %.3fms", webAssemblyToX86 - start);
	console.log("== Compilation Successful == ");
	return instance.exports;
}

if (ENABLE_TEST_CASES) {
	const test_cases = [
		['int main() { return 0; }', 0],
		[' int main() { return 42; }', 42],
		['int main() { return 5+20-4; }', 21],
		['int main()   {  return 12     + 34   - 5; }', 41],
		['int main() { return 5+6*7; }', 47],
		['int main() { return -10+20; }', 10],
		['int main() { return - -10; }', 10],
		['int main() { return - - +10; }', 10],
		['int main() { return 27 == 27; }', 1],
		['int main() { return 1 != 32; }', 1],
		['int main() { return 2 * 50   >=    200 / 2   ; }', 1],
		['int main() { return 2 > 1; }', 1],
		['int main() { return (2 > 1) * 8 \n\n; }', 8],
		['int main() { return - 1 == - 1; }', 1],
		['int main() { return 0 >= - 1; }', 1],
		['int main() { return -1 > -129; }', 1],
		['int main() { 1; return 2; }', 2],
		['int main() { int a = 1; }', 0],
		['int main() { int a = 3; return a; }', 3],
		['int main() { int a = 15; int b = 25 * 3; return b - a + 1; }', 61],
		['int main() { int test_var = 14; return test_var; }', 14],
		['int main() { int var1 = 30; int var2 = 32; return var2 - var1; }', 2],
		['int main() { int var1 = 30; int var2 = 32; return -(var2 - var1) + 10; }', 8],
		['int main() { return 1; 2; 3; }', 1],
		['int main() { 1; return 2; 3; }', 2],
		['int main() { 1; 2; return 3; }', 3],
		['int main() { int a = 16; int b = 2; return (a * b) - 8; 1024 - 67; }', 24],
		['int main() { {1; {2;} return 3; }}', 3],
		['int main() { if (1 > 0) return 5; }', 5],
		['int main() { if (0) return 5; else return 27; }', 27],
		['int main() { int a = 5; int b = 10; if (a * b == 50) return 28; else return 92;}', 28],
		['int main() { int a = 5; int b = 10; if (a * b == 52) { return a; } else { return b; }}', 10],
		['int main() { int a = 5; int b = 10; if (a * b == 52) { return a; } else { a = 10; return b; }}', 10],
		['int main() { int a = 5; {1; { a = 2;} return 3; }}', 3],
		['int main() { for (int i = 0; i < 5; i = i + 1); return i; }', 5],
		['int main() { for (int i = 0; i < 5;) { i = i + 1; } return i; }', 5],
		['int main() { for (;;) { return 3; } }', 3],
		['int main() { for (;;) { return 3; } return 5; }', 3],
		['int main() { int i = 0; for (;;) { 2 + 2; i = i + 1; if (i > 10) return i; } }', 11],
		['int main() {\n\tint i = 0;\n\twhile (i < 5) {\n\t\ti = i + 1;\n\t}\n\treturn i;\n}', 5],
		['int main() { int x = 3; return *&x; }', 3],
		['int main() { int x = 3; int *y = &x; int z = &y; x = x + 1; return **z; }', 4],
		['int main() { int x = 0; int *y = &x; *y = 1; return x; }', 1],
		['int main() { int x = 10; int *y = &x; *y = 22; return x; }', 22],
		['int main() { return simpleFunction(); }\n int simpleFunction() { return 89; }', 89],
		['int main() { return twoPlusTwo() + 1; } int twoPlusTwo() { return 2 + 2; }', 5],
		['int twoPlusTwo() { return 2 + 2; } int main() { return twoPlusTwo() + 1; }', 5],
		// ['{ return add(1, 2); }', 3],
		// ['{ return sub(10, 5); }', 5],
		// ['{ return add(ret15(), 2); }', 17],
		// ['{ return add(add(5, 5), sub(10, 8)); }', 12],
	];

	void async function() {
		let i = 0;
		for (; i < test_cases.length; ++i) {
			let compile_result = null;
			try {
				const program = await compile(test_cases[i][0]);
				compile_result = program.main();
			} catch (e) {
				console.log("== Failed Test Case ==\nCompiler generated invalid code or threw an exception");
				console.log(e);
				debugger;
				const program = await compile(test_cases[i][0]);
				compile_result = program.main();
			}
			if (compile_result != test_cases[i][1]) {
				console.log("== Failed Test Case ==\n'%s' should return %d", test_cases[i][0], test_cases[i][1]);
				console.log("Actual Result: %d", compile_result);
				break;
			}
		}
		if (i == test_cases.length) {
			console.clear();
			console.log("== All Test Cases Passed ==");
		}
	}();
}
