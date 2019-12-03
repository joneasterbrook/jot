
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

@REM For the benefit of anyone looking at all these 'jot -tty -st -in="...' lines
@REM You're probably thinking - why not just use echo and have done with it.
@REM Well, you'd be interested to know there is an atom of method in all this madness - 
@REM I do most of the day-to-day testing in wine and pre-release testing in a genuine windows.
@REM Unfortunately, the echo in wine doesn't seem to redirect properly.
@REM
@REM So, you say, why not have a simple wrapper in a linux shell script and let that
@REM do all the echoing. Yes but linux shell scripts don't work native windows installations
@REM and, for testing, it's important to avoid having anything installed that other
@REM users might not have. I've made some exceptions like wbin tools and gnu libraries
@REM Now I could have a dos batch file wrapper for windows and a unix-shell wrapper for
@REM linux but then the testing is not *guaranteed* to be consistent across platforms.
@REM So, until someone comes up with a good suggestion I'll stick with the devil I know.

@rm -f test1.txt test2.ref test2.txt test2a.ref test2a.txt test3.txt test3a.txt test4.txt test4a.txt test5.txt ^
    test6.ref test6.txt test7.txt test7.ref test7a.txt test7a.ref test8.jot test8.ref test8.txt test9.txt test10.txt  ^
    test10.jot test10.txt test11.txt test12.jot test12.txt

@echo on
@set JotExe=jot_dev
@set failedTests=
@rm -f test_win.txt
@touch test_win.txt
@goto Test1
 
:Test1
@echo Test  1: test.jot - The basic jot-command regression test - test.jot
@%JotExe% t.t -in="%%r=test; m-k-0mk0 %%o=test1.txt; %%a0"
@grep "Successfully completed all tests" test1.txt > NULL
@if NOT errorlevel 1 goto Test2
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 1

:Test2
@echo Test  2: streaming out to stdout in -tty mode.
@rm -f Test.txt Test2.ref
@%JotExe% l99.t -st -tty -quiet -init="(v/__9/pm,m)0 %%a" > test2.txt
@tail -10 l99.t > test2.ref
@diff test2.ref test2.txt
@if NOT errorlevel 1 goto Test3
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 2

:Test3
@echo Test  3: Accepting input from stream in -tty mode.
@rm -f test3.txt
@%JotExe% - < t.t -quiet -tty -init="%%o=test3.txt; %%a"
@diff test3.txt t.t
@if NOT errorlevel 1 goto Test3a
  @echo **** Fail ****
  @set failedTests=%failedTests% 3

:Test3a
@echo Test  3a: Accepting input from stream - non-tty mode.
@rm -f test3a.txt
@%JotExe% - < t.t -quiet -tty -init="%%o=test3a.txt; %%a"
@diff test3a.txt t.t
@if NOT errorlevel 1 goto Test4
  @echo **** Fail ****
  @set failedTests=%failedTests% 3a

:Test4
@rm -f test4.txt
@echo Test  4: Accepting input from stream and piping to stdout in -tty mode.
@%JotExe% - < t.t -quiet -tty -init="p10 %%a" > test4.txt
@diff test4.txt t.t
@if NOT errorlevel 1 goto Test5
  @echo **** Fail ****
  @set failedTests=%failedTests% 4

:Test5
@echo Test  5: Read from CLI command.
@rm -f test5.txt
@%JotExe% t.t -tty -quiet -in="%%eq=cat l99.t; %%o=test5.txt; %%a"
@diff test5.txt l99.t
@if NOT errorlevel 1 goto Test6
  @echo **** Fail ****
  @set failedTests=%failedTests% 5

:Test6
@REM redundant - was set up to test -filter mode.

:Test7
@echo Test  7: streamed-in commands in -tty mode.
@Rem First construct the command script.
@jot test7.jot -new -tty -st -in="i.m-0 (i/zzz /m2m-)0.bi.m-0(r0i/ zzz/m2m-)0.bi.%%c. %%C"
@%JotExe% t.t -to=test7.txt -tty -quiet -obey < test7.jot
@%JotExe% t.t -to=test7.ref -tty -quiet -in="(i/zzz /r0i/ zzz/m2m-)0 %%c"
@diff -q test7.txt test7.ref
@if NOT errorlevel 1 goto Test7a
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
@if NOT errorlevel 1 goto Test8
  @echo **** Fail ****
  @set failedTests=%failedTests% 7a

:Test8
@echo Test  8: piping stdout in -tty mode.
@%JotExe% t.t -tty -quiet -init="f/1234567890/p%%a" > test8.txt
@tail -1 test8.txt | grep "3 :1234567890" > NULL
@if NOT errorlevel 1 goto Test8a
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 8

:Test8a
@echo Test  8a: piping stdout non-tty mode
@%JotExe% t.t -quiet -init=f/1234567890/p%%a > test8a.txt
@tail -1 test8a.txt | grep "3 :1234567890" > NULL
@if NOT errorlevel 1 goto Test9
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
jot test9.jot -new -tty -st -in="i@%%%%A few simple edits to start.@b %%c;"
jot test9.jot -tty -st -in="m0i@m-0 (i/zzz /r0i/ zzz/m2m-)0@b %%c;"
jot test9.jot -tty -st -in="m0i@m-0f/jon/@b %%c;"
jot test9.jot -tty -st -in="m0i@%%ea=ls ${JOT_RESOURCES}/docs@b %%c;"
jot test9.jot -tty -st -in="m0i@z.mha@b %%c;"
jot test9.jot -tty -st -in="m0i@%%ib=test9.txt@b %%c;"
jot test9.jot -tty -st -in="m0i@z.mhb@b %%c;"
jot test9.jot -tty -st -in="m0i@m-0f/jon/i/.../@b %%c;"
jot test9.jot -tty -st -in="m0i@%%%%Remove Bill's folly from text.@b %%c;"
jot test9.jot -tty -st -in="m0i@n.a#z#ol13oo/%%c/ z.m-0(f'#e)0@b %%c;"
jot test9.jot -tty -st -in="m0i@%%o=test9.txt;@b %%c;"
jot test9.jot -tty -st -in="m0i@%%qa=pid; ki.%%e=kill .r0i.;. 'a@b %%c;"
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
@if NOT errorlevel 1 goto Test10
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 9

:Test10
@echo -asConsole scripts test.
@REM Set up the command script.
jot test10.jot -new -tty -st -in="i@%%%%Test 10 command script.@b %%c"
jot test10.jot -tty -st -in="m0i@%%d^=^[Xd0       m-0f/jon/i/##/@b %%c;"
jot test10.jot -tty -st -in="m0i@ @b %%c;"
jot test10.jot -tty -st -in="m0i@%%m=******************************************** Test10;@b %%c;"
jot test10.jot -tty -st -in="m0i@%%m=Test -asConsole is working correctly@b %%c;"
jot test10.jot -tty -st -in="m0i@z.@b %%c;"
jot test10.jot -tty -st -in="m0ol27oo@Congratulations %%cXd0@b %%c;"
jot test10.jot -tty -st -in="m0i@%%c;@b2  %%c;"
%jotExe% t.t -to=test10.txt -st -init="%%w 15; %%w 0; %%r=./test10.jot -asConsole;"
@head -4 test10.txt | tail -1 | grep "4 :Test file for Congratulations jonathans wonderful editor." > NULL
@if NOT errorlevel 1 goto Test11
  @echo "**** Fail ****"
  @set failedTests=%failedTests% 10

:Test11
@echo Test  11:  get.jot.
@REM Use the get.jot self-test routine.
@%JotExe% %JOT_RESOURCES%/test_get/hello.c -init="%%r=get; %%h'=call GetSelfTest;" > test11.txt
@tail -1 test11.txt | grep "get.jot seems OK" > NULL
@if NOT errorlevel 1 goto Test12
  @echo **** Fail ****
  @set failedTests=%failedTests% 11

:Test12
echo "Test 12 type-to-screen mode tests."
@REM 
@REM In a new line, insert a string then rubout last three characters **** NB in linuxland rubout is ^? not ^[ (windows)
@REM Set up the command script.
\rm test12.jot
  
jot test12.jot -new -tty -in="m0i@f/jon/@b %%c;"
@REM Set up ^ key-translation buffer.
jot test12.jot -tty -st -in="m0i/%%g^/b %%c;"
jot test12.jot -tty -st -in="m0i/^[X0        %%s=commandmode +2/b %%c;"
jot test12.jot -tty -st -in="m0i@^[ins       i/##/@b %%c;"
jot test12.jot -tty -st -in="m0i@:@b %%c;"
@REM Enter type-into-screen mode and insert "zzz".
jot test12.jot -tty -st -in="m0i@X0xyz@b %%c;"
jot test12.jot -tty -st -in="m0i@123999@(r0%%~=08;)4b %%c;"
@REM Return to command mode.
jot test12.jot -tty -st -in="m0i@abcdefghiX0           @b %%c;"
  
@REM Verify.
jot test12.jot -tty -st -in="m0i@%%g#@b %%c;"
jot test12.jot -tty -st -in="m0i@%%%%Verification for test 12.@b %%c;"
jot test12.jot -tty -st -in="m0i@m-0@b %%c;"
jot test12.jot -tty -st -in="m0i@( v/1 :abcdefghijklmnopqrstuvwxyz/@b %%c;"
jot test12.jot -tty -st -in="m0i@  mv/2 :ABCDEFGHIJKLMNOPQRSTUVWXYZ/@b %%c;"
jot test12.jot -tty -st -in="m0i@  mv/3 :1234567890/@b %%c;"
jot test12.jot -tty -st -in="m0i@  mv/4 :Test file for xyz/r0v-//@b %%c;"
jot test12.jot -tty -st -in="m0i@  mv/123/r0v-//@b %%c;"
jot test12.jot -tty -st -in="m0i@  mv/abcdefghijonathans wonderful editor./r0v-//@b %%c;"
jot test12.jot -tty -st -in="m0i@  mv/5 :aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/@b %%c;"
jot test12.jot -tty -st -in="m0i@  m0i/Pass/b %%c;, m0i/Fail/b %%c; )@b %%c;"
jot test12.jot -tty -st -in="m0i@:@b %%c;"
jot test12.jot -tty -st -in="m0i@'#@b %%c;"

%jotExe% %JOT_RESOURCES%/t.t -to=test12.txt -st -init="%%r=./test12.jot -asConsole;"
@tail -1 test12.txt | grep Pass > NULL
@if NOT errorlevel 1 goto CheckFails
  @echo **** Fail Test 12 ****
  @set failedTests=%failedTests% 12
  @cat t.t


:CheckFails
@echo
@echo
@echo
@if "%failedTests%" == "" goto Pass
  @jot test_win.txt -new -in="i/=========== The windows version failed the following tests: %failedTests% ==========/ %%c"
@cat test_win.txt
@exit /b 1

:Pass
  @jot test_win.txt -new -in="i/ ===================== The windows version passed all tests. ==================== / %%c"
@cat test_win.txt
@exit /b 0

