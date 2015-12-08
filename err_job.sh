#!/bin/sh
set -x

#'remove' non exist job id
./xproducer -j remove -t 99
#'priority' non exist job id
./xproducer -j priority -t 99 -p 3

#'checksum' file not exist
./xproducer -j checksum -t /tmp/test/nonexistfile

#'concatenate' input file not exist
./xproducer -j concatenate -t /tmp/test/out -x /tmp/test/nonexistfile
#'concatenate' output file already exist
./xproducer -j concatenate -t /tmp/test/out -x /tmp/test/compress

#'compress' input file not exist
./xproducer -j compress -t /tmp/test/nonexistfile
#'compress' output file already exist
./xproducer -j compress -t /tmp/test/compress -x /tmp/test/testChecksum

#'decompress' input file not ending with .z
./xproducer -j decompress -t /tmp/test/compress 
