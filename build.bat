@echo off

set OutputFileName=jwin
set ApplicationSrcMainFile=..\stars.cpp

set DisableOptimization=-Od
set GenerateDebugInfo=-Z7
set ConfigFilePath=/I..

set IncludeDirectories=/I..\jwin %ConfigFilePath%

set OutputFiles=/Fm%OutputFileName%.map /Fe%OutputFileName%.exe /Fo%OutputFileName%.obj
set LinkFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib shcore.lib
REM -fp:fast -fp:except- -GR- -EHa- -Zo -Oi -WX -W4 -wd4018 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4706 -FC -Z7 -GS- -Gs9999999 -wd4702
set CommonLinkerFlags=%LinkFlags%
set CommonCompilerFlags=%IncludeDirectories% /DEBUG:FULL -nologo %DisableOptimization% %GenerateDebugInfo% /std:c++17
set CommonCompilerFlags=%CommonCompilerFlags% -DJWIN_SLOW -DJWIN_INTERNAL
REM "USER_APPLICATION_TMP.dll"
  

IF NOT EXIST build mkdir build
pushd build
..\jwin\ctime\ctime -begin jwin_main.ctm
del *.pdb > NUL 2> NUL
REM echo create lock file
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=0 %ApplicationSrcMainFile%  -Fm%OutputFileName%.map -MTd -LD /link -incremental:no -opt:ref  %CommonLinkerFlags% -PDB:%OutputFileName%_%random%.pdb -EXPORT:ApplicationUpdateAndRender
set LastError=%ERRORLEVEL%
del lock.tmp
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=1  ..\jwin\win32\win32_main.cpp -Fmwin32_main.map /link %CommonLinkerFlags%

..\jwin\ctime\ctime -end jwin_main.ctm %LastError%
popd

