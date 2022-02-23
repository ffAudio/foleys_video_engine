cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

file (GLOB libs "@FFMPEG_SOURCE_DIR@/lib*")

file (COPY ${libs} @SOURCE_DIR@/config.h @FFMPEG_SOURCE_DIR@/compat
      DESTINATION @FFMPEG_OUTPUT_DIR@/include
      FILES_MATCHING PATTERN *.h
)
