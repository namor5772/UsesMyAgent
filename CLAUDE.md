# Project Memory

## Purpose

This repository contains a minimal native C++ Win32 desktop application. It opens a standard resizable Windows GUI window, displays `Hello World!` centered in the client area, and persists the window's position, size, and maximized state in `HKEY_CURRENT_USER\Software\UsesMyAgent\HelloWorld`.

## Environment and setup

- Host platform: Windows.
- IDE/build system: Visual Studio solution and MSBuild.
- Installed Visual Studio root on the current machine: `C:\Program Files\Microsoft Visual Studio\18\Community`.
- C++ platform toolset: `v145`.
- Windows SDK: `10.0.26100.0`.
- Language standard: C++17.
- Target architecture: x64.
- No external dependencies are required.

## Build commands

Run commands from the repository root.

Debug build:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' '.\HelloWorld.sln' /m /p:Configuration=Debug /p:Platform=x64 /verbosity:minimal
```

Release build:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' '.\HelloWorld.sln' /m /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
```

Build outputs:

- Debug: `bin\Debug\HelloWorld.exe`
- Release: `bin\Release\HelloWorld.exe`

## Verification

After a meaningful source or project-file change:

1. Build both `Debug|x64` and `Release|x64`.
2. Launch the executable and verify that the main window title is `Hello World`.
3. Verify that the child static control displays `Hello World!`.
4. Close the application after GUI verification.
5. Run `git diff --check` before reporting completion.

There is no unit-test framework in this minimal project. A successful warning-clean build plus the GUI smoke test is the current verification procedure.

## Project structure

- `HelloWorld.cpp` — Win32 entry point, window procedure, and centered greeting control.
- `HelloWorld.vcxproj` — Debug and Release x64 build configuration.
- `HelloWorld.sln` — Visual Studio solution.
- `README.md` — user-facing build instructions.
- `.gitignore` — excludes Visual Studio and generated build artifacts.
- `bin\` — generated executables and symbols; do not edit or commit.
- `obj\` — generated intermediate files; do not edit or commit.

## Code conventions

- Keep the application native Win32 C++ unless the requested task explicitly changes the technology.
- Use Unicode Win32 APIs and wide strings (`...W` functions and `L"..."` literals).
- Keep compiler warnings at `/W4`, SDL checks enabled, and conformance mode enabled.
- Use four-space indentation and Allman-style braces, matching `HelloWorld.cpp`.
- Prefer `constexpr` for fixed values and keep implementation-only symbols in an anonymous namespace.
- Handle Win32 API failures explicitly where they affect startup or control creation.
- Keep changes minimal and avoid adding dependencies for functionality available in the Win32 API.
- Do not commit generated files under `bin\`, `obj\`, or `.vs\`.
