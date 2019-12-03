

@if not "%1" == "-fromStick" goto TestFor_toStick
  @REM Download from memory stick.
  @if not exist source\jot.c goto WrongDir
  @set stick=E:\
  @if not exist %stick% set stick=K:\
  @if not exist %stick% goto NoStick
  @cd c:\users\jone
  @cp -p  --verbose --update %stick%\ed\source\* ed\.\source
  @cp -p  --verbose --update %stick%\ed\source\backup\* ed\.\source\backup
  @cp -p  --verbose --update %stick%\ed\source\old_backup\* ed\.\source\old_backup
  @cp -p  --verbose --update %stick%\ed\releasenote.txt ed\.\
  @cp -p  --verbose --update %stick%\ed\README ed\.\
  @cp -p  --verbose --update %stick%\ed\test.csh ed\.\
  @cp -p  --verbose --update %stick%\ed\sample*.c ed\.\
  @cp -p  --verbose --update %stick%\ed\backup\* ed\.\backup
  @cp -p  --verbose --update %stick%\ed\docs\* ed\.\docs
  @cp -p  --verbose --update %stick%\ed\docs\backup\* ed\.\docs\backup
  @cp -pR --verbose --update %stick%\ed\resources ed\.
  @cp -pR --verbose --update %stick%\resources resources
  @cp -p  --verbose --update %stick%\ed\coms\* ed\.\coms
  @cp -p  --verbose --update %stick%\ed\coms\backup\* ed\.\coms\backup
  @cp -p  --verbose --update %stick%\ed\bin\ced ed\bin\
  @cp -p  --verbose --update %stick%\ed\bin\ced.bat ed\bin\
  @cp -p  --verbose --update %stick%\work\* work
  @cp -p  --verbose --update %stick%\ed\lin32\jot ed\lin32
  @cp -p  --verbose --update %stick%\ed\lin64\jot ed\lin64
  @cp -p  --verbose --update %stick%\ed\win32\jot.exe ed\win32
  @cp -p  --verbose --update %stick%\ed\win64\jot.exe ed\win64
  @goto end

:TestFor_toStick
@if not "%1" == "-toStick" goto TestFor_compile
  @REM Upload to memory stick.
  @if not exist source\jot.c goto WrongDir
  @set stick=E:\
  @if not exist %stick% set stick=K:\
  @if not exist %stick% goto NoStick
  @chmod -R u+w %stick%
  @cd c:\users\jone
  @cp -p  --verbose --update ed\source\* %stick%\ed\source
  @cp -p  --verbose --update ed\source\backup\* %stick%\ed\source\backup
  @cp -p  --verbose --update ed\source\old_backup\* %stick%\ed\source\old_backup
  @cp -p  --verbose --update ed\test.csh README releasenote.txt releasenote_cumulitive.txt sample*.c %stick%\ed\
  @cp -p  --verbose --update ed\backup\* %stick%\ed\backup
  @cp -p  --verbose --update ed\docs\* %stick%\ed\docs
  @cp -p  --verbose --update ed\docs\backup\* %stick%\ed\docs\backup
  @chmod -R u+w %stick%\ed\resources
  @cp -pr --verbose --update ed\resources %stick%\ed\resources
  @chmod -R u+w %stick%\resources
  @cp -pr --verbose --update resources %stick%\resources
  @cp -p  --verbose --update ed\coms\* %stick%\ed\coms
  @cp -p  --verbose --update ed\coms\backup\* %stick%\ed\coms\backup
  @cp -p  --verbose --update ed\bin\ced %stick%\ed\bin\
  @cp -p  --verbose --update ed\bin\ced.bat %stick%\ed\bin\
  @cp -p  --verbose --update work\* %stick%\ed\work
  @cp -p  --verbose --update ed\jot.exe %stick%\ed
  @cp -p  --verbose --update ed\lin32\jot %stick%\ed\lin32
  @cp -p  --verbose --update ed\lin64\jot %stick%\ed\lin64
  @cp -p  --verbose --update ed\win32\jot.exe %stick%\ed\win32
  @cp -p  --verbose --update ed\win64\jot.exe %stick%\ed\win64
  @goto end

:TestFor_compile
@if not "%1" == "" goto InvalidQual
  @REM Compile script for jot in windows.
  @REM Environmet setup derived from an initial run of "c:\Program Files\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat" 
   
  @if not exist source\jot.c goto WrongDir
  @set path=C:\Program Files\Microsoft Visual Studio 10.0\Common7\IDE\;^
C:\Program Files\Microsoft Visual Studio 10.0\VC\BIN;^
C:\Program Files\Microsoft Visual Studio 10.0\Common7\Tools;^
C:\Windows\Microsoft.NET\Framework\v4.0.30319;^
C:\Windows\Microsoft.NET\Framework\v3.5;^
C:\Program Files\Microsoft Visual Studio 10.0\VC\VCPackages;^
C:\Program Files\Microsoft SDKs\Windows\v7.0A\bin\NETFX 4.0 Tools;^
C:\Program Files\Microsoft SDKs\Windows\v7.0A\bin;^
C:\bin;^
C:\Program Files\Common Files\Microsoft Shared\Windows Live;^
C:\Windows\system32;^
C:\Windows;^
C:\Windows\System32\Wbem;^
C:\Windows\System32\WindowsPowerShell\v1.0\;^
C:\strawberry\c\bin;^
C:\strawberry\perl\site\bin;^
C:\strawberry\perl\bin 
   
  @set lib=C:\Program Files\Microsoft Visual Studio 10.0\VC\LIB;C:\Program Files\Microsoft SDKs\Windows\v7.0A\lib;
   
  @set include=C:\Program Files\Microsoft Visual Studio 10.0\VC\INCLUDE;^
C:\Program Files\Microsoft SDKs\Windows\v7.0A\include;^
wine\libgw32c-0.4\include\glibc;
   
  $ for /f "usebackq delims=" %i in (`uname -a`) do set uname=%i
  $ for /f "usebackq delims=" %i in (`date /t`) do set date=%i
  $ for /f "usebackq delims=" %i in (`time /t`) do set time=%i
  set uname=WindowsNT delljanuary1374 1 6 x86
  
  @rm jot.exe
  @cl source\jot.c /Zi /D__value=_value /D_DEBUG /DVC /DVERSION_STRING="\"jot v1.6_dev, %uname%, %date% %time%\""^
    /link /debug /out:jot.exe wine\libgw32c-0.4\lib\libgw32c.a > cl.lis
  @cat cl.lis
  @ls -lrt source\jot.c jot.exe

@goto End

:InvalidQual
@echo
@echo Invalid qualifier %1 - -toStick -fromStick or nothing
@echo     ced             To recompile jot source for windows
@echo     ced -toStick    To upload selected files to memory stick E:
@echo     ced -fromStick  To download selected files from memory stick E:
@goto end 

:NoStick
@echo Can't identify your memory stick
@goto end

:WrongDir
@echo Can't see the source directory - you're in the wrong directory
@goto End
 
:end
 
