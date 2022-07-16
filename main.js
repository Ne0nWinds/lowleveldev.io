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
		const output = await WebAssembly.instantiate(code);
		console.log(output.instance.exports.main());
	}
};

const test_cases = [
	'0;', 0,
	'27;', 27,
	'1 + 5;', 6,
	'5 * 5;', 25,
	'10 / 2;', 5,
	'5 * 5 + 1;', 26,
	'1 + 5 * 5;', 26,
	'(1 + 5) * 5;', 30,
	'((1 + 5) * 5);', 30,
	'-1;', -1,
	'-1 * -1;', 1,
	'-2048;', -2048,
	'-(50 + 50 * 2);', -150,
	'-(50 + 50 * 2) + 75;', -75,
	'75 + -(50 + 50 * 2);', -75,
	'1 + (12 + 2 * 9) / 2;', 16,
	'1 + (12 + 2 * 9) / 2 + ((6 + 10) / 2);', 24,
	'1 == 1;', 1,
	'1 != 1;', 0,
	'1 >= 1;', 1,
	'1 <= 1;', 1,
	'1 > 1;', 0,
	'1 < 1;', 0,
	'1024 > 512;', 1,
	'1024 >= 512;', 1,
	'1024 < 512;', 0,
	'1024 <= 512;', 0,
	'2 * 5 == 5 * 2;', 1,
	'2 * 5 >= 5 * 2;', 1,
	'2 * 5 <= 5 * 2;', 1,
	'2 * 5 != 5 * 2;', 0,
	'2 * 5 > 5 * 2;', 0,
	'2 * 5 < 5 * 2;', 0,
	'3 * 5 > 5 * 2;', 1,
	'3 * 5 >= 5 * 2;', 1,
	'3 * 5 < 5 * 2;', 0,
	'3 * 5 <= 5 * 2;', 0,
	'3 * 5 != 5 * 2;', 1,
	'3 * 5 == 5 * 2;', 0,
	'-3 * 5 > 5 * 2;', 0,
	'-3 * 5 < 5 * 2;', 1,
	'2 * 2 < 3 * 5 + 1;', 1,
	'2 * 2 > 3 * 5 + 1;', 0,
	'2 * 2 * -1 + 768;', 764,
	'(1024 * 2 - 512) + 89;', 1625,
	'(1024 * 2 - 512) + 89 > 2 * 2 * -1 + 768;', 1,
	'(1024 * 2 - 512) + 89 < 2 * 2 * -1 + 768;', 0,
	'(2 * 2 < 3 * 5 + 1) * 8;', 8,
	'1; 2; 3;', 3,
	'2 * 5; (1024 * 2 - 512) + 89 < 2 * 2 * -1 + 768; 1;', 1,
];
console.clear();

let test_case_failure = false;
for (let i = 0; i < test_cases.length; i += 2) {
	const output = await WebAssembly.instantiate(compile(test_cases[i]));
	const result = output.instance.exports.main();
	if (result != test_cases[i + 1]) {
		console.log(`test case failed\n${test_cases[i]}\nshould return: ${test_cases[i + 1]}\nresult: ${result}`);
		test_case_failure = true;
	}
}

if (!test_case_failure) {
	console.log("All test cases passed!");
}

const error_test_cases = [
	'0',
	'1 + * 5;',
	'1 + > 5;',
	'(1024 * 2 - 512 + 89 < 2 * 2 * -1 + 768;',
	'1 + 5 +',
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
