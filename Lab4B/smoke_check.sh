#!/bin/bash

./lab4b --period=4 --scale=C --log="LOGFILE" <<-EOF
SCALE=F
PERIOD=2
START
STOP
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "Return Code Failed"
fi

#Temperature check
egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]+\.[0-9]' LOGFILE &> /dev/null; \
if [ $? -ne 0 ]
then
	echo "Temperature Check Failed"
fi

if [ ! -s LOGFILE ]
then
	echo "Log File not created"
fi

#Logfile info check
egrep "START" LOGFILE &> /dev/null;
if [ $? -ne 0 ]
then
	echo "START not found in logfile"
fi

egrep "OFF" LOGFILE &> /dev/null;
if [ $? -ne 0 ]
then 
  echo "OFF not found in logfile"
fi

egrep "STOP" LOGFILE &> /dev/null;
if [ $? -ne 0 ]
then
  echo "STOP not found in logfile"
fi

egrep "PERIOD=2" LOGFILE &> /dev/null;
if [ $? -ne 0 ]
then
	echo "PERIOD not found in logfile"
fi

egrep "SCALE=F" LOGFILE &> /dev/null;
if [ $? -ne 0 ]
then
  echo "SCALE not found in logfile"
fi

egrep "SHUTDOWN" LOGFILE &> /dev/null
if [ $? -ne 0 ]
then
	echo "SHUTDOWN not found in logfile"
fi

rm -f LOGFILE

./lab4b --thisisntanarg &> /dev/null;
if [ $? -ne 1 ]
then
	echo "Didn't exit with correct exit code"
fi

