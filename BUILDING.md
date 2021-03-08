> Shell commands in this document are given in [PowerShell](https://github.com/PowerShell/PowerShell)

# Dependencies

## CMake
You will need to  [download and install CMake](https://cmake.org/download/). Please double-check the minimum CMake version in the root `CMakeLists.txt` file of this project. The version should be right at the top:

```cmake
cmake_minimum_required (VERSION 3.19)
```

## Perl
You will need Perl for Windows for running code generation scripts. We tested building ESE with [Strawberry Perl](https://strawberryperl.com/) installed.

You will need Perl to be in your path, e.g.:

```powershell
$env:Path += ";C:\Strawberry\perl\bin\"
```

Depending on your setup, you may need to **unset** the `PERL5LIB` environment variable, e.g.:

```powershell
Remove-Item Env:\PERL5LIB
```

## Message Compiler: Windows SDK
The Message Compiler (`mc.exe`) is included in the [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/).

You will need the Windows SDK to be in your path, e.g.:

```powershell
$env:Path += ";C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64"
```

## CMake Generator Kit: Visual Studio
We tested the following kits:
    - `Visual Studio Enterprise 2019 Release - amd64`
    - `Visual Studio Build Tools 2017 Release - amd64`

Please see the [Visual Studio Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators) article on cmake.org for more.

# CMake CLI
Once you have the dependencies installed, you can use CMake to build ESE on the command line. Note that, to prevent the CMake-generated files from appearing throughout the source tree, we build "out-of-source" by telling CMake to use the `./build/` directory.

## Configure

You will first need to *configure* the project. The exact arguments will depend on your CMake generator. Assuming your clone of ESE is in `c:\Extensible-Storage-Engine` and you're using the `Visual Studio Enterprise 2019 Release - amd64` kit, here's an example command:

```powershell
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -Hc:/Extensible-Storage-Engine -Bc:/Extensible-Storage-Engine/build -G "Visual Studio 16 2019" -T host=x64 -A x64
```

## Build
Build just the `BstfUnitTest` static library target:

```powershell
cmake --build c:/Extensible-Storage-Engine/build --config Debug --target BstfUnitTest -- /maxcpucount:10
```

Build just the `BookStoreSample` binary and all of its dependencies:
```powershell
cmake --build c:/Extensible-Storage-Engine/build --config Debug --target BookStoreSample -- /maxcpucount:10
```

Build everything:
```powershell
cmake --build c:/Extensible-Storage-Engine/build --config Debug --target ALL_BUILD -- /maxcpucount:10
```

# Visual Studio Code

If you would like to use [Visual Studio Code](https://code.visualstudio.com/) to build ESE, here's a list of helpful extensions:
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) - for building using CMake from withing Visual Studio Code
- [CMake language support](https://marketplace.visualstudio.com/items?itemName=twxs.cmake) - for editing of CMake files (autocomplete, documentation, syntax highlighting, etc.)
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) - you will get language services (go-to-definition, etc.) once you build the code with CMake