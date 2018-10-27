
@REM This script is only useful in wine.
@REM
@REM Every path will probably need changing for some other installation
@REM In particular those dangling off the stub Z:/home/jone/ed/win which is a symbolic link to the windows partition.
@REM

@set versionString=%2%
@echo Compiling, versionString=%versionString% > jot.lis

@set path=^
z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/bin/HostX86/x86;^
Z:/home/jone/bin;^
Z:/home/jone/ed/bin;^
Z:/home/jone/ed/wine/wbin;
  
@set include=^
Z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/include;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/ucrt;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/shared;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/um;
@set include=%include%;Z:/home/jone/ed/wine/libgw32c-0.4/include/glibc;

@set lib=^
Z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/lib/x86;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/lib/10.0.16299.0/ucrt/x86;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x86;
@REM Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x64;

@set uname=WindowsNT delljanuary1374 1 6 x86
@rm -f ../bin/jot_dev.exe ../bin/jot_dev.pdb  ../bin/jot_dev.ilk

@cl Z:/home/jone/ed/source/jot.c /Zi /D__value=_value /D_DEBUG /DVC ^
  /DVERSION_STRING=\"%VersionString%\"^
  /DNoRegEx ^
  /link /map:jot.map /out:jot_dev.exe ^
  Z:/home/jone/ed/wine/libgw32c-0.4/lib/libgw32c.a ^
  Z:/home/jone/ed/wine/MSDks/v7.1/lib/*.lib ^
  Z:/home/jone/ed/wine/lib/libgcc.lib >> jot.lis

@REM  Z:/home/jone/ed/wine/MSDks/v7.1/lib/*.lib ^

@ls -l %JOT_RESOURCES%/source/jot.c ./jot_dev.*



