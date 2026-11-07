# Scientific Calculator — C Backend + Web Frontend

A menu/button-driven scientific calculator website. The browser UI is built with HTML, CSS and JavaScript; all mathematical operations are performed by the C backend over HTTP.

## Features
- Arithmetic: `+`, `-`, `×`, `÷`, power, modulo and parentheses
- Trigonometry: sin, cos, tan and inverse trig
- Hyperbolic functions: sinh, cosh, tanh
- Logs: log base 10 and natural log
- Roots, square, cube, factorial, absolute value, reciprocal and percentage
- π and e constants
- DEG/RAD mode
- Calculation history, keyboard input and error handling

## Requirements
- GCC or another C compiler
- Windows: MinGW-w64 GCC (or MSYS2)
- Linux/macOS: GCC/Clang

## Run on Windows (MinGW)
Open Command Prompt in the project folder:

```bash
gcc server.c -o calculator_server.exe -lws2_32 -lm
calculator_server.exe
```


## Run on Linux
```bash
gcc server.c -o calculator_server -lm
./calculator_server
```


Keep the terminal window running while using the website. Press Ctrl+C to stop the server.

## Project structure
```text
scientific_calculator_c/
├── server.c
├── README.md
└── public/
    ├── index.html
    ├── style.css
    └── script.js
```

## Backend API
`POST /api/calculate`

Example request:
```json
{"op":"sin","a":30,"b":0,"angle":"DEG"}
```

Example response:
```json
{"ok":true,"result":0.5}
```

Supported operation names include:
`add`, `subtract`, `multiply`, `divide`, `power`, `mod`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `sqrt`, `cbrt`, `square`, `cube`, `log`, `ln`, `exp`, `factorial`, `abs`, `reciprocal`, `percent`, `pi`, `e`.

## Notes / limitations
- Trigonometric functions use the selected DEG/RAD mode. Inverse trig returns degrees in DEG mode and radians in RAD mode.
- The frontend expression parser supports arithmetic, parentheses, unary minus, powers, modulo and constants. Scientific functions are applied using the dedicated buttons to the current displayed expression.
- This is an educational local project server, not a hardened public production web server. Do not expose it directly to the public internet.
