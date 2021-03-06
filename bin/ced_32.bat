
@REM This script is only for use in wine.
@REM note new args: sourceDir, sourceFile, versionString, debug, preprocOpt
@REM      old args: sourceDir,             versionString, debug, preprocOpt

echo ced_32 args:%1 %2 %3 %4
@set sourceDir=%1
@if not "%sourceDir%" == "" goto SourceDirSet
  set sourceDir=z:/home/jone/ed/source/
:SourceDirSet
@set sourceFile=%2
@if not "%sourceFile%" == "" goto SourceFileSet
  set sourceFile=jot.c
:SourceFileSet
set versionString=%3%
set debug=%4
set preprocOpt=%5
echo DOSland sourceDir=%sourceDir% > Z:/home/jone/ed/source/jot.lis
echo DOSland versionString=%versionString% >> Z:/home/jone/ed/source/jot.lis
echo DOSland debug=%debug% >> Z:/home/jone/ed/source/jot.lis
echo DOSland preprocOpt=%preprocOpt% >> Z:/home/jone/ed/source/jot.lis
echo ced_32 args:%1 %2 %3 %4 >> Z:/home/jone/ed/source/jot.lis

echo DOSland "sourceDir=%sourceDir%"
echo DOSland "versionString=%versionString%"
echo DOSland debug=%debug%
echo DOSland preprocOpt=%preprocOpt%

@set path=^
Z:/home/jone/ed/wine_32/MVS10.0/VC/bin;^
Z:/home/jone/ed/wine_32/wbin;^
Z:/home/jone/ed/bin;^
Z:/home/jone/bin;^
Z:/home/jone/ed/wine_32/MVS10.0/VC/bin;^
Z:/home/jone/ed/wine_32/Windows Kits/8.0/Debuggers/x86;^
Z:/home/jone/ed/wine_32/MVS10.0/Common7/IDE;^
Z:/home/jone/ed/wine_32/pdc34dllw;
  
@set include=^
Z:/home/jone/ed/wine_32/MVS10.0/VC/include;^
z:/home/jone/ed/wine_32/MSDKs/v7.1/Include;^
z:/home/jone/ed/wine_32/libgw32c-0.4/include/winx;^
Z:/home/jone/ed/wine_32/libgw32c-0.4/include/glibc;
  
@set lib=^
Z:/home/jone/ed/wine_32/MVS10.0/VC/lib;^
Z:/home/jone/ed/wine_32/MSDKs/v7.1/Lib;^
Z:/home/jone/ed/wine_32/MVS10.0/Common7/IDE;

for /F %%i%% in ("%sourceFile%") do set exeName=%~ni%.exe
if "%sourceFile%"=="jot.c" @set exeName=jot_dev.exe
@echo Compiling %sourceDir%%sourceFile% to %exeName%
  
@rm -f bin/%exeName%
@REM  @cl %sourceDir%%sourceFile% /Zi /D__value=_value /D_DEBUG /DVC /DNoRegEx ^
@REM  /link /debug /out:Z:\home\jone\bin\%exeName%^
@cl %sourceDir%%sourceFile%  /D__value=_value /D_DEBUG /DVC /DNoRegEx ^
  /DVERSION_STRING=\"%VersionString%\" /DNoRegEx %debug% %preprocOpt% ^
  /link /out:Z:\home\jone\bin\%exeName% ^
    Z:\home\jone\ed\wine_32\libgw32c-0.4\lib\libgw32c.a ^
    Z:\home\jone\ed\wine_32\MSDKs\v7.1\Lib\*.Lib ^
    Z:\home\jone\ed\wine_32\lib\libgcc.lib >> Z:\home\jone\ed\source\jot.lis
    
@Z:/home/jone/ed/wine_32/wbin/date
@ls -lrt --full-time Z:/home/jone/ed/source/jot.c Z:/home/jone/bin/jot_dev.*
@grep -i warning Z:/home/jone/ed/source/jot.lis
@grep -i error Z:/home/jone/ed/source/jot.lis
