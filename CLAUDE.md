# Project Memory

## Purpose

This repository contains a minimal native C++ Win32 desktop calculator. It provides a read-only display above a resizable button grid for decimal input, clear, backspace, addition, subtraction, multiplication, division, and equals. The Settings menu offers optional custom colours: Button Colours recolours the calculator buttons, Background Colours recolours the client-area background around the controls (not the display, menu bar, or title bar), Output Colours recolours the read-only display's background, and Reset Colours restores the defaults. The window's position, size, and maximized state and any chosen colours are persisted in `HKEY_CURRENT_USER\Software\UsesMyAgent\HelloWorld`.

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
2. Launch the executable and verify that the main window title is `Calculator`.
3. Verify that the read-only display starts at `0`, the calculator button grid is visible, and basic arithmetic works.
4. If colour handling changed, verify the Settings menu commands: Button Colours recolours the buttons, Background Colours recolours only the client-area background, Output Colours recolours only the display background, and Reset Colours restores the defaults.
5. Close the application after GUI verification.
6. Run `git diff --check` before reporting completion.

There is no unit-test framework in this minimal project. A successful warning-clean build plus the GUI smoke test is the current verification procedure.

## Project structure

- `HelloWorld.cpp` — Win32 entry point, calculator state and operations, controls, responsive layout, settings menu with colour persistence, and window procedure.
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
