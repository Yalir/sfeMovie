#!/bin/sh
set -e

os=$(uname)

if [ "${os}" = "Darwin" ]
then
	# Add dependencies
    brew install yasm wget

    # Download SFML
    wget -q https://www.sfml-dev.org/files/SFML-2.4.2-osx-clang.tar.gz
    tar -xzf SFML-2.4.2-osx-clang.tar.gz
    export SFML_ROOT=$(pwd)/SFML-2.4.2-osx-clang/Frameworks
else
	# Add dependencies
	sudo apt-get update -qq
	sudo apt-get install -yq yasm wget
	
	# Download SFML
	wget -q https://www.sfml-dev.org/files/SFML-2.4.2-linux-gcc-64-bit.tar.gz
	tar -xzf SFML-2.4.2-linux-gcc-64-bit.tar.gz
	export SFML_ROOT=$(pwd)/SFML-2.4.2
fi
