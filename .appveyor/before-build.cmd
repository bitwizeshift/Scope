SETLOCAL EnableDelayedExpansion

@REM  # Install 'conan' dependencies

mkdir build
cd build
conan install ..

@REM  # Generate the CMake project

cmake .. -A%PLATFORM% -DSCOPE_ENABLE_UNIT_TESTS=On || exit /b !ERRORLEVEL!