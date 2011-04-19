#! /bin/sh
### BEGIN INIT INFO
# Provides:          staging.churnalism.com
# Required-Start:    $local_fs $remote_fs $network $syslog
# Required-Stop:     $local_fs $remote_fs $network $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# X-Interactive:     true
# Short-Description: Start/stop superfastmatch server example
### END INIT INFO
#----------------------------------------------------------------
# Startup script for the server of Kyoto Tycoon
#----------------------------------------------------------------


# configuration variables
prog="superfastmatch"
cmd="/usr/local/bin/ktserver"
basedir="/var/run/superfastmatch"
port="1978"
tout="30"
thnum="8"
pidfile="$basedir/pid"
logfile="/var/logs/superfastmatch.log"
miscopts="-onr -scr  /path/to/search.lua"
#ulogdir="$basedir/ulog"
#ulim=1g
#sid=1
#mhost="anotherhost.localdomain"
#mport="1978"
#rtsfile="$basedir/rts"
dbname="/path/to/index.kct#ktopts=p#msiz=48g#pccap=4g#dfunit=8"
retval=0


# setting environment variables
LANG=C
LC_ALL=C
PATH="$PATH:/sbin:/usr/sbin:/usr/local/sbin"
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
export LANG LC_ALL PATH LD_LIBRARY_PATH


# start the server
start(){
  printf 'Starting the SuperFastMatch server\n'
  mkdir -p "$basedir"
  if [ -z "$basedir" ] || [ -z "$port" ] || [ -z "$pidfile" ] || [ -z "$dbname" ] ; then
    printf 'Invalid configuration\n'
    retval=1
  elif ! [ -d "$basedir" ] ; then
    printf 'No such directory: %s\n' "$basedir"
    retval=1
  else
    if [ -f "$pidfile" ] ; then
      pid=`cat "$pidfile"`
      printf 'Killing existing process: %d\n' "$pid"
      kill -TERM "$pid"
      sleep 1
    fi
    cmd="$cmd -port $port -tout $tout -th $thnum -dmn -pid $pidfile"
    if [ -n "$logfile" ] ; then
      cmd="$cmd -log $logfile -li"
    fi
    if [ -n "$ulogdir" ] ; then
      cmd="$cmd -ulog $ulogdir -ulim $ulim -sid $sid"
    fi
    if [ -n "$miscopts" ] ; then
      cmd="$cmd $miscopts"
    fi
    if [ -n "$mhost" ] ; then
      cmd="$cmd -mhost $mhost -mport $mport -rts $rtsfile"
    fi
    cmd="$cmd $dbname"
    printf "Executing: %s\n" "$cmd"
    $cmd
    if [ "$?" -eq 0 ] ; then
      printf 'Done\n'
    else
      printf 'The server could not started\n'
      retval=1
    fi
  fi
}


# stop the server
stop(){
  printf 'Stopping the SuperFastMatch server\n'
  if [ -f "$pidfile" ] ; then
    pid=`cat "$pidfile"`
    printf "Sending the terminal signal to the process: %s\n" "$pid"
    kill -TERM "$pid"
    c=0
    while true ; do
      sleep 0.1
      if [ -f "$pidfile" ] ; then
        c=`expr $c + 1`
        if [ "$c" -ge 100 ] ; then
          printf 'Hanging process: %d\n' "$pid"
          retval=1
          break
        fi
      else
        printf 'Done\n'
        break
      fi
    done
  else
    printf 'No process found\n'
    retval=1
  fi
}


# send HUP to the server for log rotation
hup(){
  printf 'Sending HUP signal to the SuperFastMatch server\n'
  if [ -f "$pidfile" ] ; then
    pid=`cat "$pidfile"`
    printf "Sending the hangup signal to the process: %s\n" "$pid"
    kill -HUP "$pid"
    printf 'Done\n'
  else
    printf 'No process found\n'
    retval=1
  fi
}


# check permission
if [ -d "$basedir" ] && ! touch "$basedir/$$" >/dev/null 2>&1
then
  printf 'Permission denied\n'
  exit 1
fi
rm -f "$basedir/$$"


# dispatch the command
case "$1" in
start)
  start
  ;;
stop)
  stop
  ;;
restart)
  stop
  start
  ;;
hup)
  hup
  ;;
*)
  printf 'Usage: %s {start|stop|restart|hup}\n' "$prog"
  exit 1
  ;;
esac


# exit
exit "$retval"



# END OF FILE