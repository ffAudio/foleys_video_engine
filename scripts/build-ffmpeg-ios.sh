#!/bin/sh
set -e

if [[ $# -ne 2 ]]; then
    echo "Please add the path to the ffmpeg repository as second argument and the architecture as third"
    exit 1
fi

mkdir -p ../libs
pushd ../libs
mkdir -p ffmpeg.build.arm64
pushd ffmpeg.build.arm64

# directories
FF_VERSION="4.3.1"
#FF_VERSION="snapshot-git"
if [[ $FFMPEG_VERSION != "" ]]; then
  FF_VERSION=$FFMPEG_VERSION
fi
SOURCE="ffmpeg-$FF_VERSION"
FAT="FFmpeg-iOS"

SCRATCH="scratch"
# must be an absolute path
THIN=`pwd`/"thin"

# absolute path to x264 library
#X264=`pwd`/fat-x264

#FDK_AAC=`pwd`/../fdk-aac-build-script-for-iOS/fdk-aac-ios

CONFIGURE_FLAGS="--enable-cross-compile --disable-debug --disable-programs \
                 --disable-doc --enable-pic"

if [ "$X264" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
fi

if [ "$FDK_AAC" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac --enable-nonfree"
fi

# avresample
#CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-avresample"

ARCHS="arm64 armv7 x86_64 i386"

COMPILE="y"
LIPO="y"

DEPLOYMENT_TARGET="11.0"
ARCHS="$2"

if [ "$COMPILE" ]
then
	if [ ! `which yasm` ]
	then
		echo 'Yasm not found'
		if [ ! `which brew` ]
		then
			echo 'Homebrew not found. Trying to install...'
                        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
				|| exit 1
		fi
		echo 'Trying to install Yasm...'
		brew install yasm || exit 1
	fi
	if [ ! `which gas-preprocessor.pl` ]
	then
		echo 'gas-preprocessor.pl not found. Trying to install...'
		(curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
			-o /usr/local/bin/gas-preprocessor.pl \
			&& chmod +x /usr/local/bin/gas-preprocessor.pl) \
			|| exit 1
	fi

	if [ ! -r $1/$SOURCE ]
	then
		echo 'FFmpeg source not found. Trying to download...'
        pushd $1
		curl http://www.ffmpeg.org/releases/$SOURCE.tar.bz2 | tar xj \
			|| exit 1
   
        popd
	fi

	CWD=`pwd`
	for ARCH in $ARCHS
	do
		echo "building $ARCH..."

		CFLAGS="-arch $ARCH"
		if [ "$ARCH" = "i386" -o "$ARCH" = "x86_64" ]
		then
		    PLATFORM="iPhoneSimulator"
		    CFLAGS="$CFLAGS -mios-simulator-version-min=$DEPLOYMENT_TARGET"
		else
		    PLATFORM="iPhoneOS"
		    CFLAGS="$CFLAGS -mios-version-min=$DEPLOYMENT_TARGET -fembed-bitcode"
		    if [ "$ARCH" = "arm64" ]
		    then
		        EXPORT="GASPP_FIX_XCODE5=1"
		    fi
		fi

		XCRUN_SDK=`echo $PLATFORM | tr '[:upper:]' '[:lower:]'`
		CC="xcrun -sdk $XCRUN_SDK clang"

		# force "configure" to use "gas-preprocessor.pl" (FFmpeg 3.3)
		if [ "$ARCH" = "arm64" ]
		then
		    AS="gas-preprocessor.pl -arch aarch64 -- $CC"
		else
		    AS="gas-preprocessor.pl -- $CC"
		fi

		CXXFLAGS="$CFLAGS"
		LDFLAGS="$CFLAGS"
		if [ "$X264" ]
		then
			CFLAGS="$CFLAGS -I$X264/include"
			LDFLAGS="$LDFLAGS -L$X264/lib"
		fi
		if [ "$FDK_AAC" ]
		then
			CFLAGS="$CFLAGS -I$FDK_AAC/include"
			LDFLAGS="$LDFLAGS -L$FDK_AAC/lib"
		fi

		TMPDIR=${TMPDIR/%\/} $1/$SOURCE/configure \
            --install-name-dir='@executable_path/Frameworks' \
            --enable-shared \
            --disable-static \
            --libdir=`pwd`/../iOS/arm64 \
            --prefix=`pwd` \
            --incdir=`pwd`/../include \
            --target-os=darwin \
            --arch=$ARCH \
		    --cc="$CC" \
		    --as="$AS" \
		    $CONFIGURE_FLAGS \
		    --extra-cflags="$CFLAGS" \
		    --extra-ldflags="$LDFLAGS" \
		|| exit 1

		make -j3 install $EXPORT || exit 1
  
      DYLIBS="avcodec avformat avutil swresample swscale"
     
      for DYLIB in $DYLIBS
      do
          # set OUT_DIR and LIB_NAME
          LIB_NAME="$DYLIB"
          FW_PATH="../iOS/arm64/$LIB_NAME.framework"
          INFO_PLIST="$FW_PATH/Info.plist"
          OUT_DYLIB="$FW_PATH/$LIB_NAME"

          # set the DYLIBS and SOURCE_INFO_PLIST for the library
          echo $PWD
          mkdir -p "$FW_PATH"
          cp "../../scripts/Info_$DYLIB.plist" "$INFO_PLIST"
          lipo "../iOS/arm64/lib$DYLIB.dylib" -output "$OUT_DYLIB" -create
          install_name_tool -id @rpath/../Frameworks/$LIB_NAME.framework/$LIB_NAME "$OUT_DYLIB"

          # set the DYLIBS and SOURCE_INFO_PLIST for DSYM
          OUT_DSYM_PATH="$FW_PATH.dSYM/Contents/Resources/DWARF"
          INFO_PLIST="$FW_PATH.dSYM/Contents/Info.plist"
          mkdir -p "$OUT_DSYM_PATH"
          cp "../../scripts/Info_$DYLIB.plist" "$INFO_PLIST"
          lipo "../iOS/arm64/lib$DYLIB.dylib" -output "$OUT_DSYM_PATH/$LIB_NAME" -create
      done
      
      #The following steps need to be performed for inter-framework referencing, as they are not .dylibs anymore
      #e.g. check: otool -L Frameworks/avformat.framework/avformat
      
      filename="../iOS/arm64/avformat.framework/avformat"
      expathval=$(otool -l $filename | grep -F "avcodec" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/avcodec.framework/avcodec $filename
      expathval=$(otool -l $filename | grep -F "swresample" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/swresample.framework/swresample $filename
      expathval=$(otool -l $filename | grep -F "libavutil" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/avutil.framework/avutil $filename

      filename="../iOS/arm64/avcodec.framework/avcodec"
      expathval=$(otool -l $filename | grep -F "swresample" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/swresample.framework/swresample $filename
      expathval=$(otool -l $filename | grep -F "libavutil" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/avutil.framework/avutil $filename

      filename="../iOS/arm64/swresample.framework/swresample"
      expathval=$(otool -l $filename | grep -F "libavutil" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/avutil.framework/avutil $filename

      filename="../iOS/arm64/swscale.framework/swscale"
      expathval=$(otool -l $filename | grep -F "libavutil" | awk -F"name "  '{print $2}' | awk -F\(  '{print $1}')
      install_name_tool -change $expathval @executable_path/Frameworks/avutil.framework/avutil $filename
    done
fi

popd
popd

echo Done
