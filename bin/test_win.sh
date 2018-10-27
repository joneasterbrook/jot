#!/bin/sh
#Launches test.bat from a linux shell.
#
cd ${JOT_HOME}
  
jotExe=jot_dev
if [ "$1" != "" ]; then
  jotExe=$1
  fi
echo "Testing $jotExe"

\wineconsole bin/test.bat ${jotExe}

failedTests=`\grep "The following tests failed:" test_win.txt`
if [ ! -z "${failedTests}" ]; then
  \echo ${failedTests}
  exit -1
else
  \echo Passed all tests.
  exit 0
  fi
