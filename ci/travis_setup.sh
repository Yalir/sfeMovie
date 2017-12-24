#!/bin/sh
set -e

brew_if_needed()
{
    if ! which "$1" > /dev/null; then
        brew install "$1"
    fi
}

linux_sound_card()
{
    # From https://github.com/k3it/qsorder/blob/master/.travis.yml
    sudo usermod -a -G audio travis
    sudo apt-get install -y -qq portaudio19-dev libasound2-dev alsa-utils alsa-oss
    current_dir="$(dirname "$(readlink -f "$0")")"
    sudo bash "${current_dir}/prep-dummy-soundcard.sh"
}

if [ "$(uname)" = "Darwin" ]
then
    # Add dependencies
    brew_if_needed yasm
    brew_if_needed wget

    # Download SFML
    cd /tmp
    wget -q https://www.sfml-dev.org/files/SFML-2.4.2-osx-clang.tar.gz
    tar -xzf SFML-2.4.2-osx-clang.tar.gz
    ln -s "$(pwd)/SFML-2.4.2-osx-clang/Frameworks" /tmp/SFML_ROOT
else
    # Add dependencies
    sudo apt-get update -qq
    sudo apt-get install -y -qq yasm wget unzip libopenal-dev libfreetype6-dev libjpeg-dev libxrandr-dev xcb libxrandr-dev mesa-common-dev libflac-dev libvorbis-dev libudev-dev
    
    # Create virtual display for later tests
    /sbin/start-stop-daemon --start --quiet --pidfile /tmp/custom_xvfb_1.pid --make-pidfile --background --exec /usr/bin/Xvfb -- :1 -screen 0 1280x1024x16

    # Create virtual sound card for later tests
    linux_sound_card

    # Download & build SFML (issues with GLIBC symbols if the provided binaries are used)
    cd /tmp
    wget -q https://www.sfml-dev.org/files/SFML-2.4.2-sources.zip
    unzip -q SFML-2.4.2-sources.zip
    mkdir sfml-build && cd sfml-build
    cmake /tmp/SFML-2.4.2 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
    make -j8
    sudo make install

    ln -s /usr/local /tmp/SFML_ROOT
fi
