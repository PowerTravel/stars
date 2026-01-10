@echo off

REM set shoudprint=true
set shoudprint=

IF NOT EXIST build mkdir build
pushd build
cl       ../vector_unit_tests.cpp       /I ..\.. /Z7 -nologo && .\vector_unit_tests.exe      %shoudprint%
cl       ../list_unit_tests.cpp         /I ..\.. /Z7 -nologo && .\list_unit_tests.exe        %shoudprint%
cl /EHsc ../n_tree_unit_tests.cpp       /I ..\.. /Z7 -nologo && .\n_tree_unit_tests.exe      %shoudprint%
cl /EHsc ../bucket_array_unit_tests.cpp /I ..\.. /Z7 -nologo && .\bucket_array_unit_tests.exe %shoudprint%
popd

