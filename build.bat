@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
echo Building elevator_final...
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "G:\Users\claude_project\算法实习\elevator_final\elevator_final.sln" /p:Configuration=Debug /p:Platform=x64 /v:minimal
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build SUCCESS!
) else (
    echo.
    echo Build FAILED!
)
