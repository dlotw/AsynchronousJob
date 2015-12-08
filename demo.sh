#!/bin/sh
set -x
# WARNING: this script doesn't check for errors, so you have to enhance it in case any of the commands
# below fail.
FILE=/tmp/test
LOGFILE=logfile

if [ ! -e "$FILE" ]
then
        mkdir /tmp/test
        cp -r ./test /tmp
else
        rm -r $FILE
        mkdir /tmp/test
        cp -r ./test /tmp
fi

if [ -e "$LOGFILE"]
then
        rm -f $LOGFILE
fi

# To test priority change and remove job, we add several checksum jobs to the queue 
./xproducer -j checksum -t /tmp/test/testChecksum
./xproducer -j checksum -t /tmp/test/testChecksum
./xproducer -j checksum -t /tmp/test/testChecksum
./xproducer -j compress -t /tmp/test/compress
./xproducer -j decompress -t /tmp/test/decompress.z
./xproducer -j compress -t /tmp/test/compress -x /tmp/test/compressOutput.z
./xproducer -j compress -t /tmp/test/compress
./xproducer -j concatenate -t /tmp/test/OutputConcat -x /tmp/test/concat1 -x /tmp/test/concat2 -x /tmp/test/concat3
./xproducer -j checksum -t /tmp/test/testChecksum
./xproducer -j list
# Set job ID=2 priority to 0, the kernel will move this job to the end of job queue
./xproducer -j priority -t 2 -p 0
./xproducer -j list
# Remove job ID=2, the kernel will remove that job
./xproducer -j remove -t 6
./xproducer -j list

# Write the results to logfile, which enable user to check their jobs
dmesg>>logfile

