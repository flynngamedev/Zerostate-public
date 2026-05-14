@echo off
echo ============================================
echo  ZeroState Build System - Desktop (x64)
echo ============================================
echo.

:: Setup MinGW-w64 path
set MINGW=D:\mingw64\bin
if not exist "%MINGW%\g++.exe" (
    echo ERROR: MinGW-w64 not found at D:\mingw64
    echo Check your MinGW installation path.
    pause
    exit /b 1
)
set PATH=%MINGW%;%PATH%
echo MinGW-w64 environment ready.
echo.

:: app.exe
echo [1/3] Building app.exe...
cd app
g++ -O2 -std=c++17 -o app.exe main.cpp ../core/physics.cpp ../core/entropy.cpp ../core/quantum.cpp -I../core -luser32 > build_log.txt 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo FAILED -- see app\build_log.txt
    cd ..
    goto done
)
echo OK
del /q build_log.txt >nul 2>&1
cd ..

:: password.exe
echo [2/3] Building password.exe...
cd password
g++ -O2 -std=c++17 -o password.exe password.cpp ../core/physics.cpp ../core/entropy.cpp ../core/quantum.cpp -I../core -luser32 > build_log.txt 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo FAILED -- see password\build_log.txt
    cd ..
    goto done
)
echo OK
del /q build_log.txt >nul 2>&1
cd ..

:: visualiser.exe
echo [3/3] Building visualiser.exe...
cd visualiser
g++ -O2 -std=c++17 -o visualiser.exe visualiser.cpp ../core/physics.cpp ../core/entropy.cpp ../core/quantum.cpp -I../core -ld3d11 -ld3dcompiler -ldxgi -luser32 -mwindows > build_log.txt 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo FAILED -- see visualiser\build_log.txt
    cd ..
    goto done
)
echo OK
del /q build_log.txt >nul 2>&1
cd ..

echo.
echo ============================================
echo  All builds successful!
echo    app\app.exe
echo    password\password.exe
echo    visualiser\visualiser.exe
echo ============================================

:done
echo.
pause
