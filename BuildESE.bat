@ECHO OFF

rem NOTE: Make sure the path does not have white spaces
set "scriptDir=%~dp0"

cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -H%scriptDir% -B%scriptDir%\build -G "Visual Studio 15 2017" -T host=x64 -A x64

cmake --build %scriptDir%/build --config Release --target ALL_BUILD -- /maxcpucount:10
