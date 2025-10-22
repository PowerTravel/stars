@echo off

IF NOT EXIST build mkdir build
pushd build
cl /DCOMPILER_MSVC ../vector_unit_tests.cpp /Z7 -nologo && .\vector_unit_tests.exe
popd

