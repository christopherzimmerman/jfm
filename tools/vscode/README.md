# JFM Language Support for Visual Studio Code

This extension provides rich language support for JFM, a Rust-like language that transpiles to C.

## Features

### Syntax Highlighting
- Complete syntax highlighting for all JFM language constructs
- Keywords: `fn`, `let`, `mut`, `struct`, `impl`, `if`, `else`, `while`, `for`, `loop`, etc.
- Types: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`, `bool`, `char`, `str`
- Operators, strings, numbers, and comments

### Code Snippets
Quick snippets for common patterns:
- `fn` - Function definition
- `main` - Main function
- `struct` - Struct definition
- `impl` - Implementation block
- `let` - Variable declaration
- `if`, `ifelse` - Conditionals
- `while`, `for`, `loop` - Loops
- `extern` - External function declaration
- `include` - Include C headers

### Language Configuration
- Auto-closing brackets and quotes
- Comment toggling (line: `//`, block: `/* */`)
- Bracket matching
- Code folding
- Auto-indentation

## Installation

### From the JFM Distribution
This extension is bundled with the JFM compiler. To install:

1. Open VS Code
2. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS)
3. Type "Install from VSIX"
4. Navigate to `jfm/vscode-extension/jfm/` and select the `.vsix` file

### Manual Installation
1. Copy the `jfm` folder to your VS Code extensions directory:
   - Windows: `%USERPROFILE%\.vscode\extensions\`
   - macOS/Linux: `~/.vscode/extensions/`
2. Restart VS Code

## Usage

Once installed, the extension will automatically activate for files with the `.jfm` extension.

## Example

```jfm
// A simple JFM program
struct Vector2 {
    x: f32,
    y: f32,
}

impl Vector2 {
    fn new(x: f32, y: f32) -> Vector2 {
        return Vector2 { x: x, y: y };
    }
    
    fn magnitude(self) -> f32 {
        return (self.x * self.x + self.y * self.y);
    }
}

fn main() -> i32 {
    let v: Vector2 = Vector2::new(3.0, 4.0);
    println("Vector created!");
    return 0;
}
```

## Language Server (Future)

A language server providing intelligent code completion, error checking, and refactoring support is planned for future releases.

## Contributing

This extension is part of the JFM language project. For issues or contributions, please visit the main JFM repository.

## License

Same as the JFM compiler - MIT License