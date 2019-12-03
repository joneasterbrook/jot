
@REM This script implements the dos tests for jot, it is separated from the checking scripts,
@REM test_win.bat and test_win.csh, which check results in dos and linux respectively.
@REM This started life as one script named test.bat but it proved impossible to make this
@REM work under wine.
  
@REM sample batch script:  /home/jone/ed/wine/MVS10.0/VC/bin/vcvars32.bat 
 
@REM N.B. GetDefaultPath assignment may need changing to suit your environment - see Test 11
  
@cd %JOT_HOME%
  
@REM Tests demonstrating various modes of operation for jot
@REM
@REM History
@REM 29/06/13 Adapted from test.csh

@echo on
@set JotExe=jot_dev
@set failedTests=
@rm -rf test1.* test2.* test3.* test3a.* test4.* test5.* test6.* test7.* test7a.* test8.* testa8.* test9.* test10.* test11.*
@goto Test9
 
:Test1
@echo Test  1: test.jot - The basic jot-command regression test - test.jot
@%JotExe% t.t -in="%%r=test; m-k-0mk0 %%o=test1.txt; %%a0"
@grep "Successfully completed all tests" test1.txt > NULL
@if errorlevel 0 goto Test2
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 1

:Test2
@echo Test  2: streaming out to stdout in -tty mode.
@rm -f Test.txt Test2.ref
@%JotExe% l99.t -st -tty -quiet -init="(v/__9/pm,m)0 %%a" > test2.txt
@tail -10 l99.t > test2.ref
@diff test2.ref test2.txt
@if errorlevel 0 goto Test3
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 2

:Test3
@echo Test  3: Accepting input from stream in -tty mode.
@rm -f test3.txt
@%JotExe% - < t.t -quiet -tty -init="%%o=test3.txt; %%a"
@diff test3.txt t.t
@if errorlevel 0 goto Test3a
  @echo **** Fail ****
  @set failedTests=%failedTests% 3

:Test3a
@echo Test  3a: Accepting input from stream - non-tty mode.
@rm -f test3a.txt
@%JotExe% - < t.t -quiet -tty -init="%%o=test3a.txt; %%a"
@diff test3a.txt t.t
@if errorlevel 0 goto Test4
  @echo **** Fail ****
  @set failedTests=%failedTests% 3a

:Test4
@rm -f test4.txt
@echo Test  4: Accepting input from stream and piping to stdout in -tty mode.
@%JotExe% - < t.t -quiet -tty -init="p10 %%a" > test4.txt
@diff test4.txt t.t
@if errorlevel 0 goto Test5
  @echo **** Fail ****
  @set failedTests=%failedTests% 4

:Test5
@echo Test  5: Read from CLI command.
@rm -f test5.txt
@%JotExe% t.t -tty -quiet -in="%%eq=cat l99.t; %%o=test5.txt; %%a"
@diff test5.txt l99.t
@if errorlevel 0 goto Test6
  @echo **** Fail ****
  @set failedTests=%failedTests% 5

:Test6
@echo Test  6: Filter mode.
@rm -f test6.txt test6.ref
%JotExe% l99.t -to=test6.txt -quiet -filter="f1/9:/,k" -init="%%c"
@jot l99.t -to=test6.ref -quiet -init="(f1/9:/m, k)0 m0r0b %%c"
@diff test6.ref test6.txt
@if errorlevel 0 goto Test7
  @echo **** Fail ****
  @set failedTests=%failedTests% 6

:Test7
@echo Test  7: streamed-in commands in -tty mode.
@Rem First construct the command script.
@jot test7.jot -new -tty -st -in="i.m-0 (i/zzz /m2m-)0.bi.m-0(r0i/ zzz/m2m-)0.bi.%%c. %%C"
@%JotExe% t.t -to=test7.txt -tty -quiet -obey < test7.jot
@%JotExe% t.t -to=test7.ref -tty -quiet -in="(i/zzz /r0i/ zzz/m2m-)0 %%c"
@diff -q test7.txt test7.ref
@if errorlevel 0 goto Test7a
  @echo **** Fail ****
  @set failedTests=%failedTests% 7

:Test7a
@echo Test  7a: streamed-in commands in non-tty mode.
@rm -f test7a.jot test7a.txt test7a.ref
@Rem First construct the command script.
@jot test7a.jot -new -tty -st -in="i.m-0 (i/zzz /m2m-)0.bi.m-0(r0i/ zzz/m2m-)0.bi.%%c. %%C"
@%JotExe% t.t -to=test7a.txt -quiet -obey < test7a.jot
@%JotExe% t.t -to=test7a.ref -tty -quiet -in="(i/zzz /r0i/ zzz/m2m-)0 %%c"
@diff -q test7a.txt test7a.ref
@if errorlevel 0 goto Test8
  @echo **** Fail ****
  @set failedTests=%failedTests% 7a

:Test8
@echo Test  8: piping stdout in -tty mode.
@%JotExe% t.t -tty -quiet -init="f/1234567890/p%%a" > test8.txt
@tail -1 test8.txt | grep "3 :1234567890" > NULL
@if errorlevel 0 goto Test8a
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 8

:Test8a
@echo Test  8a: piping stdout non-tty mode
@%JotExe% t.t -quiet -init=f/1234567890/p%%a > test8a.txt
@tail -1 test8a.txt | grep "3 :1234567890" > NULL
@if errorlevel 0 goto Test9
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 8a

:Test9
@echo Test  9: journal-keeping/recovery test.
@chmod -R u+wx test9.txt.jnl
@rm -rf test9.txt.jnl
@rm -f test9.txt test9.ref
@cp t.t test9.txt
@chmod ugo+w test9.txt
@REM
@REM test9.jot is set up this way because wine doesn't echo properly and doesn't do continuation lines properly.
@REM
@echo %%%%A few simple edits to start. > test9.jot
@echo m-0 (i/zzz /r0i/ zzz/m2m-)0 >> test9.jot
@echo m-0f/jon/ >> test9.jot
@echo %%ea=ls ${JOT_RESOURCES}/docs >> test9.jot
@echo z.mha >> test9.jot
@echo %%ib=test9.txt >> test9.jot
@echo z.mhb >> test9.jot
@echo m-0f/jon/i/.../ >> test9.jot
@echo %%%%Remove Bill's folly from text. >> test9.jot
@echo n.a#z#ol13oo/%%c/ z.m-0(f'#e)0 >> test9.jot
@echo %%o=test9.txt; >> test9.jot
@echo %%qa=pid; ki.%%e=kill .mk 'a >> test9.jot
@REM
@%JotExe% test9.txt -journal -obey < test9.jot
@REM
@REM Nobbling the history file.
@%JotExe% test9.txt.jnl/history.txt -st -in="f/%%qa=pid; ki.%%e=kill / r-0i.%%%%.mi.. %%C"
@REM 
@REM Recover the session.
@mv test9.txt test9.ref
@cp t.t test9.txt
@echo zzz>>test9.txt
@chmod ugo+w test9.txt
@%JotExe% test9.txt -quiet -st=recover -init=%%C
@diff test9.ref test9.txt > NULL
@if errorlevel 0 goto Test10
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 9

:Test10
@echo -asConsole scripts test.
@REM Set up the command script.
@echo %%%%Test 10 command script. > test10.jot
@echo %%d^=Xd0     m-0f/jon/i/##/ >> test10.jot
@echo   >> test10.jot
@echo %%m=******************************************** Test10; >> test10.jot
@echo %%m=Test -asConsole is working correctly >> test10.jot
@echo z. >> test10.jot
@echo Congratulations %%cXd0      >> test10.jot
@echo %%c; >> test10.jot
@REM
sleep 10
%jotExe% t.t -to=test10.txt -st -init="%%w 15; %%w 0; %%r=./test10.jot -asConsole;"
head -4 test10.txt | tail -1 | grep "4 :Test file for Congratulations jonathans wonderful editor." > NULL
@if errorlevel 0 goto Test11
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 10

:Test11
@goto AllDone
@echo Test  11:  get.jot.
@REM Use the get.jot self-test routine.
@%JotExe% %JOT_RESOURCES%/test_get/hello.c -init="%%r=get; %%h'=call GetSelfTest;" > test11.txt
@tail -1 test11.txt | grep "get.jot seems OK" > NULL
@if errorlevel 0 goto CheckFails
  @echo **** Fail ****
  @set failedTests=%failedTests% 11

:CheckFails
@echo
@echo
@echo
@if "%failedTests%" == "" goto Pass
  @echo *******************The following tests failed: %failedTests% *****************
@sleep 60
@exit /b 1

:Pass
@echo ******************************* Passed all tests. ******************************
@exit /b 0

