# UsesMyAgent — Native Windows Calculator

A small, dependency-free native C++ desktop calculator for Windows. It provides a read-only output display above a familiar button grid and restores the window's last position, size, and maximized state when it is started again.

This repository also serves as a compact example of a local, AI-assisted coding workflow: the source, Visual Studio project, builds, GUI smoke tests, Git operations, and project-specific instructions are all managed directly in a Windows checkout.

## Features

- Native Win32 GUI with no console window.
- Read-only output display above a familiar, resizable calculator button grid.
- A Settings menu in the window's menu bar with Button Colours and Background Colours commands (placeholders until their behaviour is wired up).
- Bold Segoe UI display and button text that automatically scales as the window is resized.
- Display text that shrinks as needed to keep long values visible.
- A centered, theme-aware owner-drawn backspace graphic.
- Decimal input, clear, backspace, addition, subtraction, multiplication, division, and equals.
- Immediate left-to-right chained calculations and clean recovery after division by zero.
- The same Windows mouse icon as the local desktop shortcut in the title bar and window UI.
- Unicode Windows API calls throughout.
- Persistent window position, size, and maximized state.
- Validation that a saved window still intersects an available monitor before restoring it.
- Separate Debug and Release x64 configurations.
- No third-party libraries, package manager, runtime framework, or installer.

## Coding environment

The project was created and verified in the following local environment:

| Component | Configuration |
| --- | --- |
| Host operating system | Windows |
| Repository checkout used for verification | A local Windows Git checkout (the path is machine-specific) |
| IDE | Visual Studio Community under `C:\Program Files\Microsoft Visual Studio\18\Community` |
| Build engine | MSBuild 18.8.2 |
| C++ platform toolset | `v145` |
| Windows SDK | `10.0.26100.0` |
| Language mode | C++17 |
| Character set | Unicode |
| Target architecture | x64 |
| Compiler checks | Warning level 4, SDL checks, and conformance mode |
| Source control | Git, with `origin` pointing to the GitHub repository |
| Shell used for automation | PowerShell |
| External dependencies | None |

The solution is intentionally based on a Visual Studio `.sln` and `.vcxproj`; CMake and Ninja are not required. Generated files are placed under `bin\` and `obj\`, and those directories are excluded from Git.

### AI-assisted local workflow

The coding agent works directly against the local repository rather than generating an isolated example elsewhere. Its workflow for this project is:

1. Read `CLAUDE.md` for durable project-specific build, architecture, style, and verification rules.
2. Inspect existing source and project files before modifying them.
3. Make focused edits that preserve the native Win32 design and existing style.
4. Build both Debug and Release x64 configurations with the installed MSBuild.
5. Launch the executable for a GUI smoke test and inspect the real window/control state.
6. For persistence changes, close and restart the process and compare its window rectangle.
7. Run `git diff --check`, inspect repository status, and only commit or push when explicitly requested.

`CLAUDE.md` is project memory for coding agents and contributors. It records machine-specific commands and conventions, while this README is the user-facing project guide.

## Repository layout

| Path | Purpose |
| --- | --- |
| `HelloWorld.cpp` | Win32 entry point, Settings menu bar, calculator state and operations, responsive controls, window procedure, and Registry-backed window placement persistence |
| `HelloWorld.vcxproj` | Visual C++ Debug and Release x64 build settings |
| `HelloWorld.sln` | Visual Studio solution |
| `CLAUDE.md` | Local project memory, verification procedure, and coding conventions |
| `.gitignore` | Excludes Visual Studio state and generated build outputs |
| `bin\` | Generated executables and PDB files; not committed |
| `obj\` | Generated compiler/linker intermediates; not committed |

## Build requirements

To build on another machine, install Visual Studio with the **Desktop development with C++** workload and a compatible Windows SDK. If that installation uses a different toolset or SDK version, Visual Studio can retarget the project when the solution is opened.

No additional dependencies need to be downloaded.

## Build in Visual Studio

1. Open `HelloWorld.sln`.
2. Select either `Debug` or `Release`.
3. Select the `x64` platform.
4. Choose **Build > Build Solution**.

The outputs are:

- `bin\Debug\HelloWorld.exe`
- `bin\Release\HelloWorld.exe`

## Build from PowerShell

From the repository root on the verified development machine:

```powershell
$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'

& $msbuild '.\HelloWorld.sln' /m /p:Configuration=Debug /p:Platform=x64 /verbosity:minimal
& $msbuild '.\HelloWorld.sln' /m /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
```

From a Visual Studio Developer PowerShell where `msbuild` is already available on `PATH`, the shorter equivalent is:

```powershell
msbuild .\HelloWorld.sln /m /p:Configuration=Release /p:Platform=x64
```

## Run

After a Release build:

```powershell
.\bin\Release\HelloWorld.exe
```

A local desktop shortcut named **Hello World - Advanced Edition** may also be created to point to the Release executable. The shortcut itself is machine-specific and is not stored in this repository.

## Window placement persistence

When the window receives a normal close request, the application stores a Win32 `WINDOWPLACEMENT` structure for the current Windows user at:

```text
HKEY_CURRENT_USER\Software\UsesMyAgent\HelloWorld
```

The value is named `WindowPlacement`. On the next launch, the application restores the saved normal window rectangle and whether the window was maximized. Minimized state is deliberately not restored, so the application never starts hidden on the taskbar.

Saved placement is ignored if it is malformed or does not intersect a currently available monitor. In that case, the application opens with its default `360 × 500` size, centered using the primary screen dimensions.

To reset the saved placement from PowerShell:

```powershell
Remove-Item 'HKCU:\Software\UsesMyAgent\HelloWorld' -Recurse -Force -ErrorAction SilentlyContinue
```

## Verification

There is no unit-test framework because the program is a very small native GUI application. The current verification procedure is:

1. Build `Debug|x64` successfully.
2. Build `Release|x64` successfully.
3. Launch the real executable and verify:
   - the main window title is `Calculator`;
   - the menu bar contains a `Settings` entry whose drop-down lists `Button Colours` and `Background Colours`;
   - the read-only display starts at `0` above the calculator button grid;
   - digits, decimals, clear, backspace, all four arithmetic operations, and equals work;
   - division by zero displays `Error`, and entering a digit starts a new calculation.
4. Move and resize the window, close it normally, and launch it again.
5. Confirm that the second process restores the same window rectangle.
6. Run:

```powershell
git diff --check
```

The persistence smoke test uses Windows process and User32 APIs from PowerShell to manipulate and inspect the actual application window; it does not mock the GUI behavior.

## Development conventions

- Keep the application native Win32 unless the project requirements explicitly change.
- Use wide strings and Unicode API variants such as `CreateWindowExW`.
- Use four-space indentation and Allman-style braces.
- Keep implementation-only constants and helpers in the anonymous namespace.
- Preserve warning level 4, SDL checks, and conformance mode.
- Handle startup-critical Win32 API failures explicitly.
- Do not commit `.vs\`, `bin\`, `obj\`, user settings, or Visual Studio database files.

## Cleaning generated output

Close the running application first, then remove the generated directories:

```powershell
Remove-Item .\bin, .\obj -Recurse -Force -ErrorAction SilentlyContinue
```

Rebuild the solution to recreate them.
