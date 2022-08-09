import compiler from "./compiler.js";

const editorElement = document.getElementById("editor");
const editor = ace.edit(editorElement);
editor.resize();

window.compiler = compiler;

const compile = (text) => {
	const code_ptr = compiler.bump_alloc_src_code(text.length + 1);
	const u8Array = new Uint8Array(compiler.memory.buffer, code_ptr, text.length + 1);
	const text_encoder = new TextEncoder('utf-8');
	text_encoder.encodeInto(text, u8Array);

	const compile_result_ptr = compiler.compile(code_ptr, u8Array.byteLength);
	if (!compile_result_ptr) return null;

	const compile_result = new Uint32Array(compiler.memory.buffer, compile_result_ptr, 2);
	const code = new Uint8Array(compiler.memory.buffer, compile_result[1], compile_result[0]);

	return code;
};

document.getElementById("run").onclick = async () => {
	const editorText = editor.getValue();

	const code = compile(editorText);
	console.log(code);
	if (code == null) return;

	if (code[0] == 40) {
		const text_decoder = new TextDecoder('utf-8');
		const output = text_decoder.decode(code);
		console.log(output);
	} else {
		window.output = await WebAssembly.instantiate(code);
		console.log(output.instance.exports.main());
	}
};

const RUN_TEST_CASES = 1;

if (RUN_TEST_CASES) {
	const test_cases = [
		'int main() { 0; }', 0,
		'int main() { 27; }', 27,
		'int main() { 1 + 5; }', 6,
		'int main() { 5 * 5; }', 25,
		'int main() { 10 / 2; }', 5,
		'int main() { 5 * 5 + 1; }', 26,
		'int main() { 1 + 5 * 5; }', 26,
		'int main() { (1 + 5) * 5; }', 30,
		'int main() { ((1 + 5) * 5); }', 30,
		'int main() { -1; }', -1,
		'int main() { -1 * -1; }', 1,
		'int main() { -2048; }', -2048,
		'int main() { -(50 + 50 * 2); }', -150,
		'int main() { -(50 + 50 * 2) + 75; }', -75,
		'int main() { 75 + -(50 + 50 * 2); }', -75,
		'int main() { 1 + (12 + 2 * 9) / 2; }', 16,
		'int main() { 1 + (12 + 2 * 9) / 2 + ((6 + 10) / 2); }', 24,
		'int main() { 1 == 1; }', 1,
		'int main() { 1 != 1; }', 0,
		'int main() { 1 >= 1; }', 1,
		'int main() { 1 <= 1; }', 1,
		'int main() { 1 > 1; }', 0,
		'int main() { 1 < 1; }', 0,
		'int main() { 1024 > 512; }', 1,
		'int main() { 1024 >= 512; }', 1,
		'int main() { 1024 < 512; }', 0,
		'int main() { 1024 <= 512; }', 0,
		'int main() { 2 * 5 == 5 * 2; }', 1,
		'int main() { 2 * 5 >= 5 * 2; }', 1,
		'int main() { 2 * 5 <= 5 * 2; }', 1,
		'int main() { 2 * 5 != 5 * 2; }', 0,
		'int main() { 2 * 5 > 5 * 2; }', 0,
		'int main() { 2 * 5 < 5 * 2; }', 0,
		'int main() { 3 * 5 > 5 * 2; }', 1,
		'int main() { 3 * 5 >= 5 * 2; }', 1,
		'int main() { 3 * 5 < 5 * 2; }', 0,
		'int main() { 3 * 5 <= 5 * 2; }', 0,
		'int main() { 3 * 5 != 5 * 2; }', 1,
		'int main() { 3 * 5 == 5 * 2; }', 0,
		'int main() { -3 * 5 > 5 * 2; }', 0,
		'int main() { -3 * 5 < 5 * 2; }', 1,
		'int main() { 2 * 2 < 3 * 5 + 1; }', 1,
		'int main() { 2 * 2 > 3 * 5 + 1; }', 0,
		'int main() { 2 * 2 * -1 + 768; }', 764,
		'int main() { (1024 * 2 - 512) + 89; }', 1625,
		'int main() { (1024 * 2 - 512) + 89 > 2 * 2 * -1 + 768; }', 1,
		'int main() { (1024 * 2 - 512) + 89 < 2 * 2 * -1 + 768; }', 0,
		'int main() { (2 * 2 < 3 * 5 + 1) * 8; }', 8,
		'int main() { 1; 2; 3; }', 3,
		'int main() { 2 * 5; (1024 * 2 - 512) + 89 < 2 * 2 * -1 + 768; 1; }', 1,
		'int main() { int x = 5; x; }', 5,
		'int main() { int abc1234 = 15; abc1234; }', 15,
		'int main() { int x = 5; int y = 16; x; }', 5,
		'int main() { int x = 5; int y = 16; y; }', 16,
		'int main() { int ab = 1024; int abc = 7; abc; }', 7,
		'int main() { int ab = 1024; int abc = 7; ab; }', 1024,
		'int main() { int a = 105; int b = 27; a > b; }', 1,
		'int main() { int a = 1 + (12 + 2 * 9) / 2 + ((6 + 10) / 2); a == 24; }', 1,
		'int main() { int x = 1; x = 6; x; }', 6,
		'int main() { int x = 1; x = x + 1; x; }', 2,
		'int main() { int x = 5; x = 7 + 5; x = x + 1; x == 13; }', 1,
		'int main() { return 15; 6; }', 15,
		'int main() { { int x = 27; return x; } }', 27,
		'int main() { int x = 27; { 2; } return x; }', 27,
		'int main() { int x = 27; { x = x + 1; { 2; } } return x; }', 28,
		'int main() { if (1) return 5; return 27; }', 5,
		'int main() { if (0) return 5; return 27; }', 27,
		'int main() { int x = 15; if (x == 15) return 1; return -1; }', 1,
		'int main() { int x = 26 * 2; if (x >= (26 * 2)) return 1; return -1; }', 1,
		'int main() { int x = 50; if (x > 25) { x = x * 2; x = x + 1; } return x; }', 101,
		'int main() { if (1) return 50; else return 100; }', 50,
		'int main() { if (0) return 50; else return 100; }', 100,
`int main() {
	int x = 50;
	if (x == 25 * 2) {
		x = 1024;
		return x;
	} else {
		x = 2048;
		return x * 2;
	}
}`, 1024,
`int main() {
	int x = 50;
	if (x == 25 * 2 - 1) {
		x = 1024;
		return x;
	} else {
		x = 2048;
		return x * 2;
	}
}`, 4096,
		'int main() { int return_value = 5; return return_value; }', 5,
`int main() {
	int j = 1;
	for (int i = 0; i < 10; i = i + 1) {
		j = j * 2;
	}
	return j;
}`, 1024,
`int main() {
	int total = 0;
	for (int i = 0; i < 5; i = i + 1) {
		for (int j = 0; j < 5; j = j + 1) {
			total = total + 1;
		}
	}
	return total;
}`, 25,
`int main() {
	int total = 0;
	for (int i = 0; i < 5; i = i + 1) {
		for (int j = 0; j < 5; j = j + 1) {
			for (int w = 0; w < 5; w = w + 1) {
				total = total + 2;
				total = total - 1;
			}
		}
	}
	return total;
}`, 125,
`int main() {
	int total = 0;
	for (int i = 0; i < 5; i = i + 1) {
		for (int j = 0; j < 5; j = j + 1) {
			for (int w = 0; w < 5; w = w + 1) {
				if (total < 100) total = total + 1;
				else total = total + 3;
			}
		}
	}
	return total;
}`, 175,
`int main() {
	int total = 0;
	for (int i = 0; i < 5; i = i + 1)
		for (int j = 0; j < 5; j = j + 1)
			for (int w = 0; w < 5; w = w + 1)
				if (total < 100)
					total = total + 1;
				else
					total = total + 3;
	return total;
}`, 175,
`int main() {
	for (int i = 0; i < 10; i = i + 1) {
		if (i == 5) return i;
	}
	return -1;
}`, 5,
`int main() {
	for (int i = 0; i < 10; i = i + 2) {
		if (i == 5) return i;
	}
	return -1;
}`, -1,
		'int main() { int i = 0; while (i < 10) i = i + 1; return i; }', 10,
		'int main() { int i = 10; while (i < 10) i = i + 1; return i; }', 10,
		'int main() { int i = 0; do { i = i + 1; } while (i < 10); return i; }', 10,
		'int main() { int i = 10; do { i = i + 1; } while (i < 10); return i; }', 11,
		'int main() { int x = 10; int y = &x; return y; }', 65536 - 4,
		'int main() { int x = 0; *(65536 - 4) = 15; return x; }', 15,
		'int main() { int x = 0; int y = 27; *(65536 - 4) = 15; return x; }', 15,
		'int main() { int x = 1; int y = 27; *(65536 - 8) = 15; return x; }', 1,
		'int main() { int x = 27; return *&x; }', 27,
		'int main() { int x = 27; return *&*&x; }', 27,
		'int main() { int x = 27; int y = *&*&x; return y; }', 27,
		'int main() { int x = 18; int y = &x; *y = 5; return x; }', 5,
		'int main() { int x = 18; int y = &x; *y = *y + 1; return x; }', 19,
		'int main() { int x = 30; int *y = &x; int *z = &y; return **z; }', 30,
`int main() {
	int x = 5;
	// x = x + 1;
	return x;
}`, 5,
`int main() {
	int x = 5;
	/* x = x + 1; */
	return x;
}`, 5,
		'int main() { int x = 5; int y = 17; int *z = &y; return *(z + 1); }', 5,
		'int main() { int x = 5; int y = 17; int *z = &y; return *(&z + 2); }', 5,
		'int main() { int x = 5; int y = 17; return *(&y + 1); }', 5,
		'int main() { int x = 5; int y = 17; return *(&x - 1); }', 17,
		'int main() { int x = 3; int y = 13; return *(&y-(-1)); }', 3,
`int main() {
	int a = 128;
	int x = 256;
	return (&a + 3) - &x + 3;
}`, 4,
		'int main() { return 5; } int x() { return 7 + 6; }', 5,
		'int x() { return 5 + 5; }\nint main() { return x(); }', 10,
`int x() {
	int j = 20;
	return j;
}
int main() {
	int j = 5;
	return x() + j;
}`, 25,
	];
	console.clear();

	let test_case_failure = false;
	for (let i = 0; i < test_cases.length; i += 2) {
		let output = null;
		try {
			output = await WebAssembly.instantiate(compile(test_cases[i]));
		} catch (e) {
			console.log(`test case caused exception\n${test_cases[i]}`);
			console.log(e);
			test_case_failure = true;
			editor.setValue(test_cases[i]);
			break;
		}
		const result = output.instance.exports.main();
		if (result != test_cases[i + 1]) {
			console.log(`test case failed\n${test_cases[i]}\nshould return: ${test_cases[i + 1]}\nresult: ${result}`);
			test_case_failure = true;
			editor.setValue(test_cases[i]);
		}
	}

	if (!test_case_failure) {
		console.log("All test cases passed!");
	}

	const error_test_cases = [
		'{ 0 }',
		// '{ 1 + * 5; }', properly testing this requires type checking
		'{ 1 + / 8; }',
		'{ 1 + 5 +; }',
		'{ 1 + > 5; }',
		'{ (1024 * 2 - 512 + 89 < 2 * 2 * -1 + 768; }',
		'{ int = 5 }',
		'{ x = 5 }',
		'{ int ab = 10; a; }',
		'{ 5;',
		'{{{ 0; }}',
		'{ int x = 27; { x = x + 1; { 2; } return x; }',
		'int x = 27; { x = x + 1; { 2; }} return x; }',
	];

	test_case_failure = false;
	for (let i = 0; i < error_test_cases.length; ++i) {
		const output = compile(error_test_cases[i]);
		if (output != null) {
			console.log(`error test case failed\n${error_test_cases[i]}`);
			test_case_failure = true;
		}
	}
	if (!test_case_failure) {
		console.log("All error test cases passed!");
	}

}
