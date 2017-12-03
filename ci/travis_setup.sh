#!/bin/sh
set -e

os=$(uname)

if [ "${os}" = "Darwin" ]
then
	# Add dependencies
    brew install yasm wget

    # Download SFML
    cd /tmp
    wget -q https://www.sfml-dev.org/files/SFML-2.4.2-osx-clang.tar.gz
    tar -xzf SFML-2.4.2-osx-clang.tar.gz
    ln -s $(pwd)/SFML-2.4.2-osx-clang/Frameworks /tmp/SFML_ROOT
else
	# Add dependencies
	sudo apt-get update -qq
	sudo apt-get install -y -qq yasm wget
	
	# Download SFML
	cd /tmp
	wget -q https://www.sfml-dev.org/files/SFML-2.4.2-linux-gcc-64-bit.tar.gz
	tar -xzf SFML-2.4.2-linux-gcc-64-bit.tar.gz
	ln -s $(pwd)/SFML-2.4.2 /tmp/SFML_ROOT
fi
