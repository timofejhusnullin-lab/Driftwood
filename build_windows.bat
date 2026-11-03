@echo off
rem Builds Driftwood VST3 + Standalone with Visual Studio 2022 (needs CMake + Git installed)
cd /d "%~dp0"
cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :err
cmake --build build --config Release --parallel || goto :err
echo.
echo ===== DONE =====
echo VST3:       %~dp0build\Driftwood_artefacts\Release\VST3\Driftwood.vst3
echo Standalone: %~dp0build\Driftwood_artefacts\Release\Standalone\Driftwood.exe
echo Copy Driftwood.vst3 to C:\Program Files\Common Files\VST3\
pause
exit /b 0
:err
echo BUILD FAILED
pause
exit /b 1
