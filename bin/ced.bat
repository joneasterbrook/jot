:TestFor_compile
@echo Compile script for jot in windows.
@REM Environment setup derived from an initial run of "c:\Program Files\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat" 
@echo "Arg is %1"

@REM Valid args:
@REM    <no args>      - Compile 32-bit executable
@REM    -fromstick     - Update local version from memory stick.
@REM    -fromallstick  - Redefine local version from memory stick - used to fix datestamps.
@REM    -tostick       - Update localmemory-stick version from local disc.
  
@if not "%1" == "" goto TestFor_fromstick
@echo Compiling jot  
@SET PATH=C:\windows\system32;C:\windows;C:\windows\system32\wbem;C:\Users\jone\ed\wine\wbin;^
C:\Users\jone\ed\wine\MVS10.0\VC\bin;C:\Users\jone\ed\wine\MVS10.0\Common7\IDE;^
C:\Users\jone\ed\wine\MVS10.0\Common7\Packages\Debugger\X64;^
C:\Users\jone\ed\wine\Windows Kits\8.0\Debuggers\x86;C:\Users\jone\ed\wine\pdc34dllw;wine\bin;^
C:\Users\jone\bin;C:\Users\jone\ed\bin
  
@SET INCLUDE=C:\Users\jone\ed\wine\MVS10.0\VC\include;C:\Users\jone\ed\wine\pdc34dllw;^
C:\Users\jone\ed\wine\libgw32c-0.4\include\winx;C:\Users\jone\ed\wine\libgw32c-0.4\include\glibc;^
C:\Users\jone\ed\wine\MSDKs\v7.1\Include
  
@SET LIB=C:\Users\jone\ed\wine\MVS10.0\Common7\Packages\Debugger\X64;C:\Users\jone\ed\wine\MVS10.0\VC\lib;^
C:\Users\jone\ed\wine\MSDKs\v7.1\Lib
  
@set uname=WindowsNT delljanuary1374 1 6 x86
@rm -f ..\bin\jot_dev.exe
  
@cl C:\Users\jone\ed\source\jot.c /Zi /D__value=_value /D_DEBUG /DVC /DNoRegEx ^
  /DVERSION_STRING="\"jot v2.1.1_dev, %uname%, %date% %time%\""^
  /link /map:C:/Users/jone/ed/source/jot.map /out:C:\Users\jone\bin\jot_dev.exe ^
  wine\libgw32c-0.4\lib\libgw32c.a wine\MSDks\v7.1\lib\*.lib wine\lib\libgcc.lib > source/jot.lis
echo Hello world  
@grep " error " C:\Users\jone\ed\source/jot.lis
@grep " warning " C:\Users\jone\ed\source/jot.lis
@echo ------------------------------------------------------------------------------------
@grep _udivdi3 source/jot.lis
@ls -lrt C:\Users\jone\ed\source\jot.c C:\Users\jone\bin\jot_dev.exe
  
@goto End

:TestFor_fromstick
echo arg is %1
@if not "%1" == "-fromstick" goto TestFor_allfromstick
  @REM Download from memory stick.
  @set stick=E:\
  @if not exist %stick% set stick=K:\
  @if not exist %stick% goto NoStick
  @cp -p  --verbose --update %stick%ed\source\* C:\Users\jone\ed\source
@REM  @cp -p  --verbose --update %stick%ed\source\backup\* C:\Users\jone\ed\source\backup
@REM  @cp -p  --verbose --update %stick%ed\source\old_backup\* C:\Users\jone\ed\source\old_backup
@REM  @cp -p  --verbose --update %stick%ed\README C:\Users\jone\ed\
  @cp -p  --verbose --update %stick%ed\READMEs\* C:\Users\jone\ed\READMEs
  @cp -p  --verbose --update %stick%ed\READMEs\releasenote.txt C:\Users\jone\ed\READMEs\
  @cp -p  --verbose --update %stick%ed\READMEs\releasenote_cumulitive.txt C:\Users\jone\ed\READMEs\
  @cp -p  --verbose --update %stick%ed\sample*.c C:\Users\jone\ed\
  @cp -p  --verbose --update %stick%ed\backup\* C:\Users\jone\ed\backup
  @cp -p  --verbose --update %stick%ed\docs\* C:\Users\jone\ed\docs
  @cp -p  --verbose --update %stick%ed\docs\backup\* C:\Users\jone\ed\docs\backup
  @cp -pR --verbose --update %stick%ed\resources C:\Users\jone\ed
  @cp -p  --verbose --update %stick%ed\coms\* C:\Users\jone\ed\coms
  @cp -p  --verbose --update %stick%ed\coms\backup\* C:\Users\jone\ed\coms\backup
  @cp -p  --verbose --update %stick%ed\bin\test.sh C:\Users\jone\ed\bin
  @cp -p  --verbose --update %stick%ed\bin\test.bat C:\Users\jone\ed\bin
  @cp -p  --verbose --update %stick%ed\bin\ced C:\Users\jone\ed\bin\
  @cp -p  --verbose --update %stick%ed\bin\ced.bat C:\Users\jone\ed\bin
  @cp -p  --verbose --update %stick%ed\bin\cedwin_32.bat C:\Users\jone\ed\bin
  @cp -p  --verbose --update %stick%ed\bin\cedwin_64.bat C:\Users\jone\ed\bin
  @cp -p  --verbose --update %stick%ed\bin\lin32\jot_dev C:\Users\jone\ed\bin\lin32
  @cp -p  --verbose --update %stick%ed\bin\lin64\jot_dev C:\Users\jone\ed\bin\lin64
  @cp -p  --verbose --update %stick%ed\bin\win32\jot_dev.exe C:\Users\jone\ed\bin\win
  @cp -p  --verbose --update %stick%projects\* C:\Users\jone\projects
  @goto end

:TestFor_allfromstick
echo arg is %1
@if not "%1" == "-fromallstick" goto TestFor_tostick
  @REM Download from memory stick.
  @set stick=E:\
  @if not exist %stick% set stick=K:\
  @if not exist %stick% goto NoStick
  @cp -p  --verbose %stick%ed\source\* C:\Users\jone\ed\source
@REM  @cp -p  --verbose %stick%ed\source\backup\* C:\Users\jone\ed\source\backup
@REM  @cp -p  --verbose %stick%ed\source\old_backup\* C:\Users\jone\ed\source\old_backup
@REM  @cp -p  --verbose %stick%ed\README C:\Users\jone\ed\
  @cp -p  --verbose %stick%ed\READMEs\* C:\Users\jone\ed\READMEs
  @cp -p  --verbose %stick%ed\READMEs\releasenote.txt C:\Users\jone\ed\READMEs\
  @cp -p  --verbose %stick%ed\READMEs\releasenote_cumulitive.txt C:\Users\jone\ed\READMEs\
  @cp -p  --verbose %stick%ed\sample*.c C:\Users\jone\ed\
  @cp -p  --verbose %stick%ed\backup\* C:\Users\jone\ed\backup
  @cp -p  --verbose %stick%ed\docs\* C:\Users\jone\ed\docs
  @cp -p  --verbose %stick%ed\docs\backup\* C:\Users\jone\ed\docs\backup
  @cp -pR --verbose %stick%ed\resources C:\Users\jone\ed
  @cp -p  --verbose %stick%ed\coms\* C:\Users\jone\ed\coms
  @cp -p  --verbose %stick%ed\coms\backup\* C:\Users\jone\ed\coms\backup
  @cp -p  --verbose %stick%ed\bin\test.sh C:\Users\jone\ed\bin
  @cp -p  --verbose %stick%ed\bin\test.bat C:\Users\jone\ed\bin
  @cp -p  --verbose %stick%ed\bin\ced C:\Users\jone\ed\bin\
  @cp -p  --verbose %stick%ed\bin\ced.bat C:\Users\jone\ed\bin
  @cp -p  --verbose %stick%ed\bin\cedwin_32.bat C:\Users\jone\ed\bin
  @cp -p  --verbose %stick%ed\bin\cedwin_64.bat C:\Users\jone\ed\bin
  @cp -p  --verbose %stick%ed\bin\lin32\jot_dev C:\Users\jone\ed\bin\lin32
  @cp -p  --verbose %stick%ed\bin\lin64\jot_dev C:\Users\jone\ed\bin\lin64
  @cp -p  --verbose %stick%ed\bin\win32\jot_dev.exe C:\Users\jone\ed\bin\win
  @cp -p  --verbose %stick%projects\* C:\Users\jone\projects
  @goto end

:TestFor_tostick
echo Fromstick???
@if not "%1" == "-tostick" goto TestFor_compile
  @REM Upload to memory stick.
  @set stick=E:\
  @if not exist %stick% set stick=K:\
  @if not exist %stick% goto NoStick
  @chmod -R u+w %stick%
  @cp -p  --verbose --update C:\Users\jone\ed\source\* %stick%ed\source
@REM  @cp -p  --verbose --update C:\Users\jone\ed\source\backup\* %stick%ed\source\backup
@REM  @cp -p  --verbose --update C:\Users\jone\ed\source\old_backup\* %stick%ed\source\old_backup
@REM  @cp -p  --verbose --update C:\Users\jone\ed\README %stick%ed\
  @cp -p  --verbose --update C:\Users\jone\ed\install.bat %stick%ed\
  @cp -p  --verbose --update C:\Users\jone\ed\READMEs\* %stick%ed\READMEs
  @cp -p  --verbose --update C:\Users\jone\ed\releasenote.txt %stick%ed\
  @cp -p  --verbose --update C:\Users\jone\ed\releasenote_cumulitive.txt %stick%ed\
  @cp -p  --verbose --update C:\Users\jone\ed\sample*.c %stick%ed\
  @cp -p  --verbose --update C:\Users\jone\ed\backup\* %stick%ed\backup
  @cp -p  --verbose --update C:\Users\jone\ed\docs\* %stick%ed\docs
  @cp -p  --verbose --update C:\Users\jone\ed\docs\backup\* %stick%ed\docs\backup
  @cp -pR --verbose --update C:\Users\jone\ed\resources %stick%ed
  @cp -p  --verbose --update C:\Users\jone\ed\coms\* %stick%ed\coms
  @cp -p  --verbose --update C:\Users\jone\ed\coms\backup\* %stick%ed\coms\backup
  @cp -p  --verbose --update C:\Users\jone\ed\bin\test.sh %stick%ed\bin
  @cp -p  --verbose --update C:\Users\jone\ed\bin\test.bat %stick%ed\bin
  @cp -p  --verbose --update C:\Users\jone\ed\bin\ced %stick%ed\bin\
  @cp -p  --verbose --update C:\Users\jone\ed\bin\cedwin_32.bat %stick%ed\bin
  @cp -p  --verbose --update C:\Users\jone\ed\bin\cedwin_64.bat %stick%ed\bin
  @cp -p  --verbose --update C:\Users\jone\ed\bin\lin32\jot_dev %stick%ed\bin\lin32
  @cp -p  --verbose --update C:\Users\jone\ed\bin\lin64\jot_dev %stick%ed\bin\lin64
  @cp -p  --verbose --update C:\Users\jone\ed\bin\win32\jot_dev.exe %stick%ed\bin\win
  @cp -p  --verbose --update C:\Users\jone\projects\* %stick%projects
  @goto end

:InvalidQual
@echo
@echo Invalid qualifier %1 - -tostick -fromstick or nothing
@echo     ced             To recompile jot source for windows
@echo     ced -tostick    To upload selected files to memory stick E:
@echo     ced -fromstick  To download selected files from memory stick E:
@goto end 

:NoStick
@echo Can't identify your memory stick
@goto end

:WrongDir
@echo Can't see the source directory - you're in the wrong directory
@goto End
 
:end
 
