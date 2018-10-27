
@REM This script is only useful in wine.
@REM
@REM Every path will probably need changing for some other installation
@REM In particular those dangling off the stub Z:/home/jone/ed/win which is a symbolic link to the windows partition.
@REM

set pathName=%1%
@echo Compiling %pathName%
@echo Compiling %pathName% > %pathName%.lis

@set path=^
z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/bin/HostX86/x86;^
  
@set include=^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/include/10.0.16299.0/ucrt;^
Z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/include;

@set lib=^
Z:/home/jone/ed/win/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.13.26128/lib/x86;^
Z:/home/jone/ed/win/Program Files (x86)/Windows Kits/10/lib/10.0.16299.0/ucrt/x86;

@cl %pathName%.c /Zi /D__value=_value /D_DEBUG /DVC /link /out:%pathName%.exe Z:/home/jone/ed/wine/MSDks/v7.1/lib/*.lib >> %pathName%.lis

@dir %pathName%.*


