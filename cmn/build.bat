@echo off

IF NOT EXIST build mkdir build
pushd build
cl /DJWIN_SLOW /DCOMPILER_MSVC ../vector_unit_tests.cpp /Z7 -nologo && .\vector_unit_tests.exe print
cl /DJWIN_SLOW /DCOMPILER_MSVC ../list_unit_tests.cpp /Z7 -nologo && .\list_unit_tests.exe print
popd

