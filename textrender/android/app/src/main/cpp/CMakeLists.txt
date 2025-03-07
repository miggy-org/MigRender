# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("miggy")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        miggy

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        miggy.cpp
        core/android.cpp
        core/anim.cpp
        core/animmanage.cpp
        core/background.cpp
        core/baseobject.cpp
        core/bbox.cpp
        core/camera.cpp
        core/extrude.cpp
        core/fileio.cpp
        core/group.cpp
        core/image.cpp
        core/jpegio.cpp
        core/lights.cpp
        core/matrix.cpp
        core/model.cpp
        core/object.cpp
        core/package.cpp
        core/pngio.cpp
        core/polygon.cpp
        core/ray.cpp
        core/rendertarget.cpp
        core/sphere.cpp
        core/texture.cpp
        core/vector.cpp
        core-ext/jsonio.cpp
        core-ext/parser.cpp
        core-ext/targets.cpp

        # JPEGLIB
        lib/jpeglib/jcapimin.c lib/jpeglib/jcapistd.c lib/jpeglib/jccoefct.c lib/jpeglib/jccolor.c
        lib/jpeglib/jcdctmgr.c lib/jpeglib/jchuff.c lib/jpeglib/jcinit.c lib/jpeglib/jcmainct.c lib/jpeglib/jcmarker.c
        lib/jpeglib/jcmaster.c lib/jpeglib/jcomapi.c lib/jpeglib/jcparam.c lib/jpeglib/jcprepct.c lib/jpeglib/jcsample.c
        lib/jpeglib/jctrans.c lib/jpeglib/jdapimin.c lib/jpeglib/jdapistd.c lib/jpeglib/jdatadst.c
        lib/jpeglib/jdatasrc.c lib/jpeglib/jdcoefct.c lib/jpeglib/jdcolor.c lib/jpeglib/jddctmgr.c lib/jpeglib/jdhuff.c
        lib/jpeglib/jdinput.c lib/jpeglib/jdmainct.c lib/jpeglib/jdmarker.c lib/jpeglib/jdmaster.c lib/jpeglib/jdmerge.c
        lib/jpeglib/jdpostct.c lib/jpeglib/jdsample.c lib/jpeglib/jdtrans.c lib/jpeglib/jerror.c lib/jpeglib/jfdctflt.c
        lib/jpeglib/jfdctfst.c lib/jpeglib/jfdctint.c lib/jpeglib/jidctflt.c lib/jpeglib/jidctfst.c
        lib/jpeglib/jidctint.c lib/jpeglib/jquant1.c lib/jpeglib/jquant2.c lib/jpeglib/jutils.c lib/jpeglib/jmemmgr.c
        lib/jpeglib/jcarith.c lib/jpeglib/jdarith.c lib/jpeglib/jaricom.c lib/jpeglib/jmemnobs.c

        # ZLIB
        lib/zlib/adler32.c lib/zlib/compress.c lib/zlib/crc32.c lib/zlib/deflate.c lib/zlib/gzclose.c lib/zlib/gzlib.c
        lib/zlib/gzread.c lib/zlib/gzwrite.c lib/zlib/infback.c lib/zlib/inffast.c lib/zlib/inflate.c lib/zlib/inftrees.c
        lib/zlib/trees.c lib/zlib/uncompr.c lib/zlib/zutil.c

        # LIBPNG
        lib/libpng-android/png.c lib/libpng-android/pngerror.c lib/libpng-android/pngget.c
        lib/libpng-android/pngmem.c lib/libpng-android/pngpread.c lib/libpng-android/pngread.c
        lib/libpng-android/pngrio.c lib/libpng-android/pngrtran.c lib/libpng-android/pngrutil.c
        lib/libpng-android/pngset.c lib/libpng-android/pngtrans.c lib/libpng-android/pngwio.c
        lib/libpng-android/pngwrite.c lib/libpng-android/pngwtran.c lib/libpng-android/pngwutil.c
        lib/libpng-android/arm/arm_init.c #lib/libpng-android/arm/filter_neon.S
        lib/libpng-android/arm/filter_neon_intrinsics.c lib/libpng-android/arm/palette_neon_intrinsics.c

        # JSONCPP
        lib/jsoncpp/json_reader.cpp lib/jsoncpp/json_value.cpp lib/jsoncpp/json_writer.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

find_library( jnigraphics-lib jnigraphics )

find_library( android-lib android )

find_library( m-lib m )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        miggy

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        ${jnigraphics-lib}
        ${android-lib}
        ${m-lib})
