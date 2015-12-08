#!/bin/sh
set -x

#missing arguments
./xproducer
#missing option flag
./xproducer list
./xproducer -j list test/empty
#invalid job type
./xproducer -j listtt
#duplicate option flag
./xproducer -j list -j list
#unexpected argument for 'list'
./xproducer -j list -t test/empty
./xproducer -j list -p 9
#missing -t argument for 'remove'
./xproducer -j remove
#wrong -t argument type for 'remove'
./xproducer -j remove -t test/empty
#missing -p argument for 'priority'
./xproducer -j priority -t 1
#wrong -t argument for 'priority'
./xproducer -j priority -t test/empty -p 9
#invalid -p argument for 'priority'
./xproducer -j priority -t 1 -p 10
#unexpected argument for 'priority'
./xproducer -j priority -t 1 -p 9 -x test/empty

#wrong -t argument type for 'checksum'
./xproducer -j checksum -t 1
#unexpected argument for 'checksum'
./xproducer -j checksum -t test/empty -p 9 -x test/empty

#wrong -t argument type for 'concatenate'
./xproducer -j concatenate -t 1
#invalid -p argument for 'concatenate'
./xproducer -j concatenate -t test/empty -p xx
#missing -x argument for 'concatenate'
./xproducer -j concatenate -t test/empty -p 9 
#multiple input file missing -x option flag for 'concatenate'
./xproducer -j concatenate -t test/empty -p 9 -x test/in1 test/in2  

#wrong -t argument type for 'compress'
./xproducer -j compress -t 1
#unexpected argument for 'compress'
./xproducer -j compress -t test/empty -p 9 -x test/compressout.z -x test/compressout2.z





