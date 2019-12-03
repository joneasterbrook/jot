
@REM  This script is only for use in genuine windows.

@set sourceDir=%1
@if not "%sourceDir%" == "" goto SourceDirSet
  set sourceDir=source
:SourceDirSet
@set versionString=%2%
@set debug=%3
@set preprocOpt=%4
@echo DOSland sourceDir=%sourceDir% > C:/Users/jone/ed/source/jot.lis
@echo DOSland versionString=%versionString% >> C:/Users/jone/ed/source/jot.lis
@echo DOSland debug=%debug% >> C:/Users/jone/ed/source/jot.lis
@echo DOSland preprocOpt=%preprocOpt% >> C:/Users/jone/ed/source/jot.lis

set path=^
C:/Users/jone/ed/wine/MVS10.0/VC/bin;^
C:/Users/jone/ed/wine/wbin;^
C:/Users/jone/ed/bin;^
C:/Users/jone/bin;^
C:/Users/jone/ed/wine/MVS10.0/VC/bin;^
C:/Users/jone/ed/wine/MVS10.0/Common7/IDE;^
C:/Users/jone/ed/wine/Windows Kits/8.0/Debuggers/x86;^
C:/Users/jone/ed/wine/pdc34dllw;
  
set include=^
C:/Users/jone/ed/wine/MVS10.0/VC/include;^
C:/Users/jone/ed/wine/MSDKs/v7.1/Include;^
C:/Users/jone/ed/wine/libgw32c-0.4/include/winx;^
C:/Users/jone/ed/wine/libgw32c-0.4/include/glibc;
  
set lib=^
C:/Users/jone/ed/wine/MVS10.0/VC/lib;^
C:/Users/jone/ed/wine/MSDKs/v7.1/Lib;^
C:/Users/jone/ed/wine/MVS10/Common7/IDE;
  
rm -f bin/jot.exe
  
cl C:\Users\jone\ed\source\jot.c /Zi /D__value=_value /D_DEBUG /DVC /DNoRegEx ^
  /DVERSION_STRING=\"%VersionString%\" /DNoRegEx %debug% %preprocOpt% ^
  /link /debug /out:C:\Users\jone\bin\jot_dev.exe^
    C:\Users\jone\ed\wine\libgw32c-0.4\lib\libgw32c.a^
    C:\Users\jone\ed\wine\MSDKs\v7.1\Lib\*.Lib^
    C:\Users\jone\ed\wine\lib\libgcc.lib >> C:\Users\jone\ed\source\jot.lis

c:\wbin\date
@ls -lrt --full-time C:/Users/jone/ed/source/jot.c C:/Users/jone/bin/jot_dev.exe
@grep -i warning C:/Users/jone/ed/source/jot.lis
@grep -i error C:/Users/jone/ed/source/jot.lis
