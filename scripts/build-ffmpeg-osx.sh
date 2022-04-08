#!/bin/bash

if [ ! `which brew` ]
then
    echo 'Homebrew not found. Trying to install...'
                ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
        || exit 1
fi

if [ ! `which yasm` ]
then
    echo 'Yasm not found'
    echo 'Trying to install Yasm...'
    brew install yasm || exit 1
fi

echo 'Removing old tap'
brew uninstall ffmpeg@4.3
brew untap $USER/ffmpeg-4-3

echo 'Installing FFmpeg 4.3.2_4'
# TODO: Add error handling for this install (will fail if already exists)
brew tap-new $USER/ffmpeg-4-3
brew extract --version=4.3 ffmpeg $USER/ffmpeg-4-3
brew install $USER/ffmpeg-4-3/ffmpeg@4.3

echo 'Copying include directory to module'
rsync -rl /usr/local/Cellar/ffmpeg@4.3/4.3.2_4/include/ ../ffmpeg/include
echo 'Copying macOS libs to module'
rsync -rl /usr/local/Cellar/ffmpeg@4.3/4.3.2_4/lib/ ../libs/MacOSX/x86_64