@echo off
REM Build script for JFM VSCode Extension
REM This packages the extension into a .vsix file for distribution

echo Building JFM VSCode Extension...

REM Check if vsce is installed
where vsce >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: vsce is not installed
    echo Please install it with: npm install -g vsce
    exit /b 1
)

REM Package the extension
vsce package

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Success! Extension packaged as jfm-1.0.0.vsix
    echo.
    echo To install in VS Code:
    echo 1. Open VS Code
    echo 2. Press Ctrl+Shift+P
    echo 3. Type "Install from VSIX"
    echo 4. Select the jfm-1.0.0.vsix file
) else (
    echo.
    echo Error: Failed to package extension
    exit /b 1
)