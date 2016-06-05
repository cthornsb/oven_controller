#!/bin/bash

COM_PORT=/dev/ttyACM0

hash avrdude 2>/dev/null
if [ $? -ne 0 ]; then
	echo " Error: Cannot locate 'avrdude'"
	exit 1
fi

if [ "$#" -ge 1 ]
then
	avrdude -patmega328p -carduino -P${COM_PORT} -b115200 -D -Uflash:w:$1:i
	exit $?
else
	echo " Invalid number of arguments"
	echo " SYNTAX: "$0" <hexFile>"
	exit 1
fi

exit 0
