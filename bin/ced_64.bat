
@REM This script is only for use in wine.

@set sourceDir=%1
@if not "%sourceDir%" == "" goto SourceDirSet
  set sourceDir=ed/source
:SourceDirSet

@set versionString=%2%
@set backup=%3
@set debug=%4
@set preprocOpt=%5
@echo DOSland sourceDir=%sourceDir% > Z:/home/jone/ed/source/jot.lis
@echo DOSland versionString=%versionString% >> Z:/home/jone/ed/source/jot.lis
@echo DOSland backup=%backup% >> Z:/home/jone/ed/source/jot.lis

@set path=^
z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/bin/HostX86/x86;^
Z:/home/jone/bin;^
Z:/home/jone/ed/bin;^
Z:/home/jone/ed/wine_32/wbin;
  
@set include=^
Z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/include;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/ucrt;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/shared;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/um;
@set include=%include%;Z:/home/jone/ed/wine_32/libgw32c-0.4/include/glibc;

@set lib=^
Z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/lib/x86;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/lib/10.0.16299.0/ucrt/x86;

@set uname=WindowsNT delljanuary1374 1 6 x86
@rm -f ../bin/jot_dev.exe ../bin/jot_dev.pdb  ../bin/jot_dev.ilk

@REM trying to get it to eat it's linux include files.
@REM  @cl Z:/home/jone/%sourceDir%/jot.c /Zi /D__USE_GNU=Yes /D__value=_value /D_DEBUG /DVC ^

@cl Z:/home/jone/%sourceDir%/jot.c /D__value=_value /D_DEBUG /DVC ^
  /DVERSION_STRING=\"%VersionString%\" ^
  /DNoRegEx ^
  %debug% %preprocOpt% ^
  /link /map:Z:/home/jone/ed/source/jot.map /out:Z:/home/jone/bin/jot_dev%backup%.exe ^
  Z:/home/jone/ed/wine_32/libgw32c-0.4/lib/libgw32c.a ^
  Z:/home/jone/ed/wine_32/MSDks/v7.1/lib/*.lib ^
  Z:/home/jone/ed/wine_32/lib/libgcc.lib >> Z:/home/jone/ed/source/jot.lis

@Z:/home/jone/ed/wine_32/wbin/date
@ls -lrt --full-time Z:/home/jone/%sourceDir%/jot.c Z:/home/jone/bin/jot_dev.exe
@grep -i warning Z:/home/jone/ed/source/jot.lis
@grep -i error Z:/home/jone/ed/source/jot.lis
