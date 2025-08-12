# JFM Compiler

A minimal, statically-typed programming language with Rust-like syntax that transpiles to readable, portable C code.

## Features

- **Rust-like syntax** - Familiar syntax for Rust developers
- **Static typing** - Compile-time type checking with explicit type annotations
- **Zero runtime overhead** - Transpiles directly to C with no runtime library
- **Type casting** - Safe type conversions using `as` keyword
- **Beautiful error messages** - Colored output with source code snippets showing exact error locations
- **Cross-platform** - Works on Windows, Linux, and macOS
- **C interoperability** - Seamlessly call C functions and use C libraries
- **Memory safety** - Immutable by default, explicit mutability with `mut`

## Installation

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/jfm.git
cd jfm

# Build the compiler
make

# Or on Windows with MinGW
mingw32-make

# Run tests
make test
```

### Requirements

- GCC or MinGW compiler
- Make (or mingw32-make on Windows)
- C11 compatible compiler

## Usage

```bash
# Compile JFM to executable (default)
jfmc program.jfm

# Generate C code only
jfmc program.jfm --c-only -o program.c

# Compile with custom output
jfmc program.jfm -o program.exe

# Keep intermediate C file when compiling to exe
jfmc program.jfm --keep-c

# View intermediate representations
jfmc program.jfm --tokens    # Show lexer output
jfmc program.jfm --ast       # Show AST
jfmc program.jfm --semantic  # Show semantic analysis
jfmc program.jfm --c         # Print generated C code to stdout

# Check syntax without generating code
jfmc program.jfm --check

# Pass flags to C compiler
jfmc program.jfm --cc-flags "-O3 -Wall"

# Get help
jfmc --help
```

## Language Guide

### Basic Types

```rust
// Primitive types (all require explicit type annotations)
let x: i32 = 42;        // 32-bit signed integer
let y: f64 = 3.14;      // 64-bit float (double)
let flag: bool = true;  // Boolean
let ch: char = 'A';     // Character
let msg: str = "Hello"; // String literal

// All integer types
i8, i16, i32, i64       // Signed integers
u8, u16, u32, u64       // Unsigned integers
f32, f64                // Floating point
```

### Variables

```rust
// Immutable by default (generates const in C)
let x: i32 = 42;

// Mutable variables
let mut count: i32 = 0;
count = count + 1;

// Type annotations are always required
let value: f64 = 3.14;  // Explicit types required
```

### Functions

```rust
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn print_message(msg: str) {
    println(msg);
}

fn main() -> i32 {
    let result: i32 = add(10, 20);
    return 0;
}
```

### Control Flow

```rust
// If statements (parentheses optional)
if x > 0 {
    println("positive");
} else if x < 0 {
    println("negative");
} else {
    println("zero");
}

// While loops
let mut i: i32 = 0;
while i < 10 {
    println(i);
    i = i + 1;
}

// For loops (range-based only)
for i in 0..10 {
    println(i);  // Prints 0 through 9
}

// Infinite loop with break/continue
loop {
    if done { break; }
    if skip { continue; }
}
```

### Structs

```rust
struct Point {
    x: f32,
    y: f32,
}

struct Rectangle {
    top_left: Point,
    width: f32,
    height: f32,
}

// Usage
fn main() -> i32 {
    let p: Point = Point { x: 10.0, y: 20.0 };
    let rect: Rectangle = Rectangle {
        top_left: Point { x: 0.0, y: 0.0 },
        width: 100.0,
        height: 50.0,
    };
    return 0;
}
```

### Implementation Blocks

```rust
struct Circle {
    center: Point,
    radius: f32,
}

impl Circle {
    fn area(self: Circle) -> f32 {
        return 3.14159 * self.radius * self.radius;
    }
    
    fn circumference(self: Circle) -> f32 {
        return 2.0 * 3.14159 * self.radius;
    }
    
    fn contains(self: Circle, p: Point) -> bool {
        let dx: f32 = p.x - self.center.x;
        let dy: f32 = p.y - self.center.y;
        let distance: f32 = sqrt(dx * dx + dy * dy);
        return distance <= self.radius;
    }
}

// Usage
fn main() -> i32 {
    let c: Circle = Circle {
        center: Point { x: 0.0, y: 0.0 },
        radius: 5.0
    };
    let area: f32 = c.area();
    println(area);
    return 0;
}
```

### Arrays

```rust
// Fixed-size arrays only
let numbers: [i32; 5] = [1, 2, 3, 4, 5];
let matrix: [[f32; 3]; 3];

// Array access
let first: i32 = numbers[0];

// Mutable arrays
let mut data: [i32; 10];
data[0] = 42;
```

### Type Casting

```rust
// Cast between numeric types using 'as'
let x: i64 = 1000;
let y: i32 = x as i32;

let f: f64 = 3.14159;
let i: i32 = f as i32;  // Truncates to 3

let small: i8 = 127;
let big: i64 = small as i64;

// Casting between signed and unsigned
let signed: i32 = -1;
let unsigned: u32 = signed as u32;  // Reinterprets bits

// Pointer casting
let ptr: *i32 = malloc(4) as *i32;
free(ptr as *u8);
```

### Pointers and References

```rust
// Raw pointers (unsafe, for C interop)
let x: i32 = 42;
let ptr: *i32 = &x;
let value: i32 = *ptr;

// References (safe pointers)
let ref_x: &i32 = &x;
let mut y: i32 = 10;
let mut_ref: &mut i32 = &mut y;
*mut_ref = 20;
```

## Error Messages

JFM provides error messages with source code context:

```
error: Type mismatch in assignment
 --> example.jfm:5:18
   |
 5 |     let x: i32 = "hello";
   |                  ^^^^^^^ expected 'i32', found 'str'

error: Undefined variable: count
 --> example.jfm:10:15
   |
10 |     let y: i32 = count + 1;
   |                  ^^^^^ not found in this scope

error: Cannot assign to immutable variable
 --> example.jfm:3:5
   |
 3 |     x = 10;
   |     ^ variable 'x' is not mutable
```

Error messages include:
- Colored output on terminals that support it
- Line numbers and column positions
- Source code snippets with error markers
- Clear, actionable error descriptions

## Examples

### Hello World

```rust
fn main() -> i32 {
    println("Hello, World!");
    return 0;
}
```

### Fibonacci

```rust
fn fibonacci(n: i32) -> i32 {
    if n <= 1 {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() -> i32 {
    let result: i32 = fibonacci(10);
    println(result);
    return 0;
}
```

### Bubble Sort

```rust
fn bubble_sort(arr: [i32; 10]) {
    let mut n: i32 = 10;
    let mut swapped: bool = true;
    
    while swapped {
        swapped = false;
        let mut i: i32 = 1;
        while i < n {
            if arr[i - 1] > arr[i] {
                // Swap elements
                let temp: i32 = arr[i - 1];
                arr[i - 1] = arr[i];
                arr[i] = temp;
                swapped = true;
            }
            i = i + 1;
        }
        n = n - 1;
    }
}
```

### Vector Math

```rust
struct Vec2 {
    x: f32,
    y: f32,
}

impl Vec2 {
    fn new(x: f32, y: f32) -> Vec2 {
        return Vec2 { x: x, y: y };
    }
    
    fn magnitude(self: Vec2) -> f32 {
        return sqrt(self.x * self.x + self.y * self.y);
    }
    
    fn normalize(self: Vec2) -> Vec2 {
        let mag: f32 = self.magnitude();
        return Vec2 {
            x: self.x / mag,
            y: self.y / mag,
        };
    }
    
    fn dot(self: Vec2, other: Vec2) -> f32 {
        return self.x * other.x + self.y * other.y;
    }
    
    fn add(self: Vec2, other: Vec2) -> Vec2 {
        return Vec2 {
            x: self.x + other.x,
            y: self.y + other.y,
        };
    }
}

fn main() -> i32 {
    let v1: Vec2 = Vec2::new(3.0, 4.0);
    let v2: Vec2 = Vec2::new(1.0, 0.0);
    
    let mag: f32 = v1.magnitude();     // 5.0
    let norm: Vec2 = v1.normalize();   // (0.6, 0.8)
    let d: f32 = v1.dot(v2);           // 3.0
    let sum: Vec2 = v1.add(v2);        // (4.0, 4.0)
    
    return 0;
}
```

## Built-in Functions

- `println(value)` - Print value with newline (supports i32, f32, f64, str, etc.)
- `print(value)` - Print value without newline
- `sqrt(x)` - Square root (works with f32 and f64)

## C Interoperability

JFM can call C functions directly

```rust
extern fn malloc(size: u64) -> *u8;
extern fn free(ptr: *u8);

fn main() -> i32 {
    let buffer: *i32 = malloc(40) as *i32;
    free(buffer as *u8);
    return 0;
}
```

## Testing

Run the test suite:

```bash
make test
# or
mingw32-make test
```

This runs:
- **Lexer tests** - Token generation
- **Parser tests** - AST construction  
- **Semantic tests** - Type checking
- **Codegen tests** - C code generation

## License

MIT License - See LICENSE file for details