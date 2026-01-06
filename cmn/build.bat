@echo off

REM set shoudprint=true
set shoudprint=

IF NOT EXIST build mkdir build
pushd build
cl /DJWIN_SLOW /DCOMPILER_MSVC ../vector_unit_tests.cpp /Z7 -nologo && .\vector_unit_tests.exe  %shoudprint%
cl /DJWIN_SLOW /DCOMPILER_MSVC ../list_unit_tests.cpp   /Z7 -nologo && .\list_unit_tests.exe  %shoudprint%
cl /DJWIN_SLOW /DCOMPILER_MSVC /EHsc ../n_tree_unit_tests.cpp /Z7 -nologo && .\n_tree_unit_tests.exe %shoudprint%
popd

