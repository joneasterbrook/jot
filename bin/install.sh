#! /bin/sh
  
# This script (un)installs jot in a linux system.
  
\uname -s | grep "Linux" > /dev/null
if [ ! $? -eq 0 ]; then
  echo "Error: this script can only work in a linux system."
  exit 1
  fi
  
if [ "$1" = "-remove" ]; then
  echo "Removing jot from your linux system."
  if [ -e ~/.bashrc ]; then
    if [ ! -w ~/.bashrc ]; then
      echo "Error: no write access to your ~/.bashrc - can't remove jot envs."
    else
      /usr/bin/jot .bashrc -tty -in="(f/ JOT_HOME /k)0 m-0(f1/ JOT_RESOURCES /k)0 m-0(f/ TERMINFO /k)0 %c;"
      fi
    fi
  if [ -e ~/.cshrc ]; then
    if [ ! -w ~/.bashrc ]; then
      echo "Error: no write access to your ~/.cshrc - can't remove jot envs."
    else
      /usr/bin/jot .bashrc -tty -in="(f/ JOT_HOME /k)0 m-0(f1/ JOT_RESOURCES /k)0 m-0(f/ TERMINFO /k)0 %c;"
      fi
    fi
  \which sudo > /dev/null
  if [ ! $? -eq 0 ]; then
    echo
    echo Your system does not support sudo, this means you must do 
    echo the following to complete the removal of the jot installation.
    echo su -                           -- Then enter the root password.
    echo \rm -rf /usr/share/jot
    echo \rm /usr/bin/jot
  else
    \sudo \rm -rf /usr/share/jot ; \rm /usr/bin/jot
    fi
  chmod -R u+w ~/jot_resources ; \rm -rf ~/jot_resources
  exit 0
  fi
  
\uname -m | grep "86" > /dev/null
if [ $? -eq 0 ]; then
  arch=lin
else if [ `\uname -m` = "armv7l" ]; then
    arch="armv7l"
  else
    echo "Error: This jot installer script is only available for armv7l and x86 series processors."
    exit 1
    fi
  fi
  
downloadPath=$0
if [ `echo ${downloadPath} | \cut -c1` != "/" ]; then
  #It's a relative path and that may not include the subpath we need.
  downloadPath="${PWD}/${downloadPath}"
  fi
downloadPath=`echo $downloadPath | sed 's/.bin.install.sh//'`
if [ -z ${downloadPath} ]; then
  echo "Error: something went wrong while attempting to determine the download path"
  exit 1
  fi
  
version=`\echo ${downloadPath} | \sed s/^.*\\\/jot\\\///`
if [ -z ${version} ]; then
  echo "Error: something went wrong while attempting to extract the version name"
  exit 1
  fi
exePath=${downloadPath}/bin/${arch}/jot  
  
if [ "$1" = "-compile" ]; then
  echo "Compiling jot"
  \chmod u+w ${downloadPath}
  \chmod u+w ${downloadPath}/bin
  versionString="jot ${version}, from ${downloadPath}/jot.c, built `\date +%d/%m/%y\ %H:%M:%S` `\uname -nspr`  "
  \cc -DLINUX -DnoX11 -Wall ${downloadPath}/source/jot.c \
    -D_FILE_OFFSET_BITS=64 -D VERSION_STRING="\"${versionString}\"" \
     -Xlinker -Map=${downloadPath}/jot.map -lncursesw -ltinfo -ldl -Xlinker --strip-all \
     -o ${downloadPath}/bin/jot 2>&1 > ${downloadPath}/jot.lis
  if [ -e ${downloadPath}/bin/jot ]; then
    exePath=${downloadPath}/bin/jot
  else
    echo
    echo "Compilation failed, installation abandoned without making any changes to your system."
    echo
    exit 1
    fi
  fi

#Move the relevant trees to the installation area.
\which sudo > /dev/null
if [ ! $? -eq 0 ]; then
  echo
  echo Your system does not support sudo, this means you must do the following 
  \echo to complete the installation.
  echo "su -                           -- Then enter the root password."
  echo cp ${exePath} /usr/bin
  if [ ! -d /usr/share/jot/${version} ]; then
    echo mkdir -p /usr/share/jot/${version}
    fi
  echo cp -pR ${downloadPath}/docs ${downloadPath}/coms /usr/share/jot/${version} 
else
  \sudo \cp -p ${exePath} /usr/bin
  \sudo \chmod ugo+x /usr/bin/jot
  \sudo \mkdir -p /usr/share/jot/${version}
  \sudo \cp -pR ${downloadPath}/docs ${downloadPath}/coms /usr/share/jot/${version}
  fi
  
#Copy the resources tree to the users home area.
if [ ! -d ~/jot_resources ]; then
  mkdir -p ~/jot_resources
  fi
if [ ! -w ~/jot_resources ]; then
  echo "Error: You do not have write permission on the directory ~/jot_resources"
  exit 1
  fi
\cp -pRu ${downloadPath}/resources/* ~/jot_resources
  
echo "export JOT_HOME=/usr/share/jot/${version}" >> ~/.bashrc
echo "export JOT_RESOURCES=~/jot_resources" >> ~/.bashrc
if [ -e /lib/terminfo ]; then
  echo export TERMINFO=/lib/terminfo >> ~/.bashrc
  fi
  
echo "setenv JOT_HOME /usr/share/jot/${version}" >> ~/.cshrc
echo "setenv JOT_RESOURCES ~/jot_resources" >> ~/.cshrc
if [ -e /lib/terminfo ]; then
  echo setenv TERMINFO /lib/terminfo >> ~/.cshrc
  fi

