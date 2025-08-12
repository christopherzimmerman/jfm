# Installing JFM VSCode Extension

## Quick Install (Development)

For development, you can use the extension directly without packaging:

1. Open VS Code
2. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS)
3. Type "Developer: Install Extension from Location"
4. Select the `jfm/vscode-extension/jfm/` folder

## Manual Install

Copy the extension folder directly to your VS Code extensions directory:

### Windows
```cmd
xcopy /E /I vscode-extension\jfm "%USERPROFILE%\.vscode\extensions\jfm"
```

### macOS/Linux
```bash
cp -r vscode-extension/jfm ~/.vscode/extensions/
```

Then restart VS Code.

## Package and Install

To create a distributable package:

1. Install vsce (once): `npm install -g vsce`
2. Navigate to the extension directory: `cd vscode-extension/jfm`
3. Package: `vsce package`
4. Install the generated `.vsix` file in VS Code

## Features

Once installed, you'll get:

- **Syntax Highlighting**: Full color coding for JFM syntax
- **Code Snippets**: Type `fn`, `struct`, `impl`, etc. for quick templates
- **Auto-indentation**: Proper indentation for blocks
- **Bracket Matching**: Highlights matching brackets
- **Comment Toggling**: `Ctrl+/` to toggle comments

## Testing

Create a test file with `.jfm` extension to verify the extension is working:

```jfm
// test.jfm
fn main() -> i32 {
    let x: i32 = 42;
    println("Hello, JFM!");
    return 0;
}
```

The syntax should be highlighted with:
- Blue keywords (`fn`, `let`, `return`)
- Green types (`i32`)
- Orange strings
- Gray comments