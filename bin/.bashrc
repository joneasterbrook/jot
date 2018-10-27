
alias h='history 40'
alias lft='ls -l --full-time'
alias xterm='xterm -rv -s -geometry 80x40 &'

ulimit -c unlimited
#grep core /proc/sys/kernel/core_pattern > /dev/null
#if [ $? -ne 0 ]; then
#  echo "/proc/sys/kernel/core_pattern is not set up for core dumps,  to fix this:"
#  echo "    sudo bash"
#  echo "    echo core > /proc/sys/kernel/core_pattern"
#  fi

export JOT_HOME=/home/jone/ed
export JOT_RESOURCES=/home/jone/ed/my_resources
export JOT_KEYBOARD=long_keys

export PS1="\\A \\W > "
export PATH=$PATH:/home/jone/ed/bin:/home/jone/bin
