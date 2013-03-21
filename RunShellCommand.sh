#!/bin/bash

# Shell script to run the command given as parameter
# If this script is called from a batch script, it is expected to have 
#

if [ "$1" == "--from-batch" ]
  then
	shift # drop --from-batch
	shift # drop mingw param
fi

# execute
$@
