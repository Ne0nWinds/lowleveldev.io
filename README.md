# lowleveldev.io
This is the future home of a low level programming tutorial series using a custom-built WebAssembly C compiler

## How to build (Windows):
- Open powershell in project directory
- Run `.\compile.ps1`
- Open a second powershell window in project directory
- Run `python -m http.server`
- Open [localhost:8000](http://localhost:8000/) in a browser
- The code typed into the textbox will be compiled into WebAssembly + output to the js dev tools console

## Current compiler features:
- Math expressions with correct order of operations
- Variables with integer and pointer types
- If statements
- For, while, and do..while loops
- Functions with parameters
- Recursion / call stack
