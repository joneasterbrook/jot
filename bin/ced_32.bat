
@REM This script is only for use in wine.

@set sourceDir=%1
@if not "%sourceDir%" == "" goto SourceDirSet
  set sourceDir=ed/source
:SourceDirSet
@set versionString=%2%
@set debug=%3
@set preprocOpt=%4
@echo DOSland sourceDir=%sourceDir% > Z:/home/jone/ed/source/jot.lis
@echo DOSland versionString=%versionString% >> Z:/home/jone/ed/source/jot.lis
@echo DOSland debug=%debug% >> Z:/home/jone/ed/source/jot.lis
@echo DOSland preprocOpt=%preprocOpt% >> Z:/home/jone/ed/source/jot.lis

@set path=^
Z:/home/jone/ed/wine/MVS10.0/VC/bin;^
Z:/home/jone/ed/wine/wbin;^
Z:/home/jone/ed/bin;^
Z:/home/jone/bin;^
Z:/home/jone/ed/wine/MVS10.0/VC/bin;^
Z:/home/jone/ed/wine/MVS10.0/Common7/IDE;^
Z:/home/jone/ed/wine/Windows Kits/8.0/Debuggers/x86;^
Z:/home/jone/ed/wine/pdc34dllw;
  
@set include=^
Z:/home/jone/ed/wine/MVS10.0/VC/include;^
z:/home/jone/ed/wine/MSDKs/v7.1/Include;^
z:/home/jone/ed/wine/libgw32c-0.4/include/winx;^
Z:/home/jone/ed/wine/libgw32c-0.4/include/glibc;
  
@set lib=^
Z:/home/jone/ed/wine/MVS10.0/VC/lib;^
Z:/home/jone/ed/wine/MSDKs/v7.1/Lib;^
Z:/home/jone/ed/wine/MVS10/Common7/IDE;
  
@rm -f bin/jot_dev.exe
  
@cl %sourceDir%\jot.c /Zi /D__value=_value /D_DEBUG /DVC /DNoRegEx ^
  /DVERSION_STRING=\"%VersionString%\" /DNoRegEx %debug% %preprocOpt% ^
  /link /debug /out:Z:\home\jone\bin\jot_dev.exe^
    Z:\home\jone\ed\wine\libgw32c-0.4\lib\libgw32c.a^
    Z:\home\jone\ed\wine\MSDKs\v7.1\Lib\*.Lib^
    Z:\home\jone\ed\wine\lib\libgcc.lib >> Z:\home\jone\ed\source\jot.lis
    
@Z:/home/jone/ed/wine/wbin/date
@ls -lrt --full-time Z:/home/jone/ed/source/jot.c Z:/home/jone/bin/jot_dev.*
@grep -i warning Z:/home/jone/ed/source/jot.lis
@grep -i error Z:/home/jone/ed/source/jot.lis
