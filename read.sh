#!/bin/bash

LOGGER_EXE=./exec/loggerUnpacker
READER_EXE=./exec/csvReader
PROCESSOR_EXE=./exec/processor

if [[ ! -f $LOGGER_EXE || ! -f $READER_EXE || ! -f $PROCESSOR_EXE ]]; then
	make
fi

hash raw2root 2>/dev/null
if [ $? -ne 0 ]; then
	echo " Error: 'raw2root' is not installed!"
	exit 1
fi

hash tgrapher 2>/dev/null
if [ $? -ne 0 ]; then
	echo " Error: 'tgrapher' is not installed!"
	exit 2
fi

if [ $# -lt 2 ]; then
	echo " Error: Invalid number of arguments"
	echo "  SYNTAX:";
	echo "   "$0" <rawData> <tarFilename>";
	exit 3
fi

echo -e "\n--Unpacking binary data--"
$LOGGER_EXE $1 tmp.csv

if [ ! -f "tmp.csv" ]; then
	echo " Error: Failed to generate file 'tmp.csv'"
	exit 4
fi

echo -e "\n--Prescanning .csv file--\n"
$READER_EXE tmp.csv

if [ ! -f "tmp.dat" ]; then
	echo " Error: Failed to generate file 'tmp.dat'"
	exit 5
fi

echo -e "\n--Converting to root--\n"
raw2root tmp.dat --names --delimiter 44

if [ ! -f "tmp.root" ]; then
	echo " Error: Failed to generate file 'tmp.root'"
	exit 6
fi

mv tmp.root data.root

echo -e "\n--Generating temperature graph--\n"
tgrapher data.root data seconds temperature --save graphs.root temp --batch --gate seconds 0 86400

echo -e "\n--Generating pressure graph--\n"
tgrapher data.root data seconds pressure --save graphs.root pres --batch --gate seconds 0 86400

if [ ! -f "graphs.root" ]; then
	echo " Error: Failed to generate file 'graphs.root'"
	exit 7
fi

echo -e "\n--Generating pdfs--\n"
$PROCESSOR_EXE data.root graphs.root

if [ ! -f "temp.pdf" ]; then
	echo " Error: Failed to generate file 'temp.pdf'"
	exit 8
fi

if [ ! -f "pres.pdf" ]; then
	echo " Error: Failed to generate file 'pres.pdf'"
	exit 9
fi

# Tar up the resulting files.
tar -cf $2 data.root graphs.root pres.pdf temp.pdf

# Cleanup
rm -f data.tmp tmp.csv tmp.dat
rm -f data.root graphs.root

exit 0
