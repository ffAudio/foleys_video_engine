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
brew uninstall ffmpeg@4.4
brew untap $USER/ffmpeg-4-4

echo 'Installing FFmpeg 4.4'
brew tap-new $USER/ffmpeg-4-4
brew extract --version=4.4 ffmpeg $USER/ffmpeg-4-4
brew install $USER/ffmpeg-4-4/ffmpeg@4.4
# unlinking this flavor of ffmpeg so its not used elsewhere and is just being used to download and locally compile this specific version
brew unlink $USER/ffmpeg-4-4/ffmpeg@4.4

echo 'Copying include directory to module'
rsync -rl /usr/local/Cellar/ffmpeg@4.4/*/include/ ../ffmpeg/include
echo 'Copying macOS libs to module'
rsync -rl /usr/local/Cellar/ffmpeg@4.4/*/lib/ ../libs/MacOSX/x86_64