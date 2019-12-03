
@REM  This script is only for use in genuine windows.

@set sourceDir=%1
@if not "%sourceDir%" == "" goto sourceDirSet
  set sourceDir=source
:sourceDirSet
@set versionString=%2%
@if not "%versionString%" == "" goto versionStringSet
  set versionString="V2.3.1 for WindowsNT"
:versionStringSet
@set debug=%3
@set preprocOpt=%4
@echo DOSland sourceDir=%sourceDir% > C:/Users/jone/ed/source/jot.lis
@echo DOSland versionString=%versionString% >> C:/Users/jone/ed/source/jot.lis
@echo DOSland debug=%debug% >> C:/Users/jone/ed/source/jot.lis
@echo DOSland preprocOpt=%preprocOpt% >> C:/Users/jone/ed/source/jot.lis

@set path=^
C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/bin/HostX86/x86;^
C:/Users/jone/bin;^
C:/Users/jone/ed/bin;^
C:/Users/jone/ed/wine/wbin;
  
@set include=^
C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/include;^
C:/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/ucrt;^
C:/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/shared;^
C:/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/um;

@set include=%include%;C:/Users/jone/ed/wine/libgw32c-0.4/include/glibc;
@REM Trying to get it to eat it's linux include files.
@REM @set include=%include%;Z:/usr/include;
  
@set lib=^
C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/lib/x86;^
C:/Program Files (x86)/Windows Kits/10/lib/10.0.16299.0/ucrt/x86;

@REM @set uname=WindowsNT delljanuary1374 1 6 x86
@rm -f ../bin/jot_dev.exe ../bin/jot_dev.pdb  ../bin/jot_dev.ilk

@cl C:/Users/jone/ed/%sourceDir%/jot.c /Zi /D__value=_value /D_DEBUG /DVC ^
  /DVERSION_STRING=\"%VersionString%\"^
  /DNoRegEx ^
  %debug% %preprocOpt% ^
  /link /map:C:/Users/jone/ed/source/jot.map /out:C:/Users/jone/bin/jot_dev.exe ^
  C:/Users/jone/ed/wine/libgw32c-0.4/lib/libgw32c.a ^
  C:/Users/jone/ed/wine/MSDks/v7.1/lib/*.lib ^
  C:/Users/jone/ed/wine/lib/libgcc.lib >> C:/Users/jone/ed/source/jot.lis

c:\wbin\date
@ls -lrt --full-time C:/Users/jone/ed/source/jot.c C:/Users/jone/bin/jot_dev.exe
@grep -i warning C:/Users/jone/ed/source/jot.lis
@grep -i error C:/Users/jone/ed/source/jot.lis
