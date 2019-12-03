
@REM
@REM This script installs/uninstalls jot in a windows system.
@REM Run it in a windows console window thusly:  <path>\install.bat
@REM                          To uninstall jot:  <path>\install.bat -remove
@REM

@REM First test that echo will pipe through to jot - when jot set the codepage this gets broken in wine sessions - for reasons yet to be determined.
@FOR /f "usebackq delims=" %%i IN (`ECHO abc`) DO SET test=%%i
@IF "%test%" == "" (
  @ECHO "ECHO is not writing to a pipe - close this window and try again"
  @GOTO end_of_script )

@SET qual=%1%
  
@SET exePathName=%~dp0%\win\jot.exe
@FOR /f "usebackq delims=" %%i IN (`ECHO %exePathName% ^| %exePathName% -in="f/\bin\/e0f-/\jot\/-e-0 p %%a;" -st -quiet`) DO SET version=%%i
@FOR /F "usebackq delims=" %%i in (`ECHO %exePathName% ^| %exePathName% -in="(f/%version%/-re0r-0p, ) %%a" -quiet -st -tty`) do set downloadPath=%%i
@FOR /F "usebackq delims=" %%i in (`ECHO %homeDrive%%HomePath% ^| %exePathName% -in="(f/\/s./.)0m-0r0e-p %%a" -quiet -st -tty`) do set SaneHomePath=%%i

@IF "%version%" == "" (
  @ECHO "Error: something went wrong while attempting to extract the version name from the script path"
  @GOTO end_of_script )
  
@IF NOT EXIST "C:\MyPrograms\jot\%version%" (
  @MD "C:\MyPrograms\jot\%version%" )

@IF "%downloadPath%" == "" (
  @ECHO "Error: something went wrong while attempting to extract the download path name from the script path"
  @GOTO end_of_script )

@IF "%qual%" == "-remove" (
  @ECHO Removing any pre-existing jot installation.
  @REM First save the original registry to backup_jot.reg
  @RMDIR /S "C:\MyPrograms\jot\%version%"
  @RMDIR /S "%HomeDrive%%HomePath%\jot_resources"
  @REG DELETE HKEY_CURRENT_USER\Environment /F /V JOT_HOME
  @REG DELETE HKEY_CURRENT_USER\Environment /F /V JOT_RESOURCES
  @FOR /f "usebackq delims=" %%i IN (`REG QUERY HKCU\Environment ^| %exePathName% ^
    -in="(f/ PATH /(f1/ REG_SZ /-(v/ /r)0e-0f1/C:\MyPrograms\jot\%%version%%\bin\/s/ /e- p, ), ) %%a;" -st -quiet`) DO SET newPath=%%i
  @IF "%newPath%" == "" (
    @REG DELETE HKEY_CURRENT_USER\Environment /F /V PATH
  ) ELSE (
    @SETX PATH %newPath% )
  @GOTO end_of_script )

@REM Move the relevant trees to the installation area.
@MD "C:\MyPrograms
@MD "C:\MyPrograms\jot
@MD "C:\MyPrograms\jot\%version%"
@@ECHO "Copying:  %downloadPath%/docs %downloadPath%/coms C:\MyPrograms\jot\%version%"
@MD "C:\MyPrograms\jot\%version%"\coms
@MD "C:\MyPrograms\jot\%version%"\docs
@MD "C:\MyPrograms\jot\%version%"\bin
@ECHO Copying Docs:
@XCOPY /Q /Y %downloadPath%docs "C:\MyPrograms\jot\%version%\docs\"
@ECHO Copying Coms:
@XCOPY /Q /Y %downloadPath%coms "C:\MyPrograms\jot\%version%\coms\"
@ECHO Copying jot executable.
@XCOPY /Q /Y %exePathName% "C:\MyPrograms\jot\%version%\bin\"
@ECHO Copying Batch scripts:
@XCOPY /Q /Y %downloadPath%bin\*.bat "C:\MyPrograms\jot\%version%\bin\"
  
@REM Copy the resources tree to the users home area.
IF EXIST %HomeDrive%%HomePath%\jot_resources GOTO UserTree_OK
  @MD %HomeDrive%%HomePath%\jot_resources
  @XCOPY /Q /S /Y %downloadPath%\resources\* %HomeDrive%%HomePath%\jot_resources
:UserTree_OK
  
@REM The jot environment.
@SETX JOT_HOME "C:\MyPrograms\jot\%version%"
@SETX JOT_RESOURCES "%HomeDrive%%HomePath%\jot_resources"
@SET newPath=
@FOR /f "usebackq delims=" %%i IN (`REG QUERY HKCU\Environment ^| %exePathName% ^
  -in=" (f/ PATH / f1/ REG_SZ /-(v/ /r)0e-0, ) (f1/C:\MyPrograms\jot\%%version%%\bin\/, r0%%u; (r-ri/;/,) p) %%a;" -st -quiet`) DO SET newPath=%%i
@IF NOT "%newPath%" == "" @SETX PATH %newPath%

:end_of_script
