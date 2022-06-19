import compiler from "./compiler.js";

const editorElement = document.getElementById("editor");
const editor = ace.edit(editorElement);
editor.resize();

window.compiler = compiler;

document.getElementById("run").onclick = () => {
	const text_encoder = new TextEncoder('utf-8');

	const editorText = editor.getValue();
	const code_ptr = compiler.bump_alloc_src_code(editorText.length + 1);
	const u8Array = new Uint8Array(compiler.memory.buffer, code_ptr, editorText.length + 1);
	text_encoder.encodeInto(editorText, u8Array);

	const len = compiler.compile(code_ptr, u8Array.byteLength);
	console.log(len);
	if (!len) return null;
};
