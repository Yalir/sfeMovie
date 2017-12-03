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
	sudo apt-get install -y -qq yasm wget unzip openal-dev freetype-dev jpeg-dev xrandr-dev xcb-dev xcb-randr-dev xcb-image-dev opengl-dev flac-dev ogg-dev vorbis-dev vorbisenc-dev vorbisfile-dev pthread-dev
	
	# Download SFML
	cd /tmp
	wget -q https://www.sfml-dev.org/files/SFML-2.4.2-sources.zip
	unzip SFML-2.4.2-linux-gcc-64-bit.tar.gz
	mkdir sfml-build && cd sfml-build
	cmake /tmp/SFML-2.4.2 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
	make -j8
	sudo make install

	ln -s /usr/local /tmp/SFML_ROOT
fi
