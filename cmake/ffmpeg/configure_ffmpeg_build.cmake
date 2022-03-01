cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

string (REPLACE "|" ";" CONFIGURE_EXTRAS_ENCODED "@FFMPEG_CONFIGURE_EXTRAS_ENCODED@")
list (REMOVE_ITEM CONFIGURE_EXTRAS_ENCODED "")
list (REMOVE_DUPLICATES CONFIGURE_EXTRAS_ENCODED)

set (CONFIGURE_COMMAND
        ./configure
        #--cc=@FFMPEG_CC@
        #--ar=@FFMPEG_AR@
        #--strip=@FFMPEG_STRIP@
        #--ranlib=@FFMPEG_RANLIB@
        #--as=@FFMPEG_AS@
        #--nm=@FFMPEG_NM@
        #--arch=@CMAKE_SYSTEM_PROCESSOR@
        --disable-static
        --enable-shared
        --enable-protocol=file
        --shlibdir=@FFMPEG_OUTPUT_DIR@
        --prefix=@FFMPEG_OUTPUT_DIR@
        ${CONFIGURE_EXTRAS_ENCODED}
)

if ("@CMAKE_SYSROOT@")
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --sysroot=@CMAKE_SYSROOT@")
endif()

if ("@FFMPEG_C_FLAGS@")
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --extra-cflags=@FFMPEG_C_FLAGS@")
endif()

if ("@FFMPEG_LD_FLAGS@")
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --extra-ldflags=@FFMPEG_LD_FLAGS@")
endif()

if ("@IOS@" OR "@ANDROID@")

        set (CONFIGURE_COMMAND 
                ${CONFIGURE_COMMAND}
                --enable-cross-compile
                --disable-programs
                --disable-doc
                --enable-pic
        )
endif()

if ("@IOS@")

        set (CONFIGURE_COMMAND 
                ${CONFIGURE_COMMAND}
                --target-os=darwin)

elseif ("@ANDROID@")

        set (CONFIGURE_COMMAND 
                ${CONFIGURE_COMMAND}
                --target-os=android)

endif()


execute_process (COMMAND ${CONFIGURE_COMMAND}
                 WORKING_DIRECTORY @FFMPEG_SOURCE_DIR@
                 COMMAND_ECHO STDOUT
                 COMMAND_ERROR_IS_FATAL ANY)
