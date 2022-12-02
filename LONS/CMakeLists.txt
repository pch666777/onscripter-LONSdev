
cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


add_definitions(
        -fsigned-char    #ARM平台下注意char 默认是无符号的，需要启用此定义
        -DLONS_DEBUG     #debug标记s

        -DSDL_JAVA_PACKAGE_PATH=com_xc_lons
        -DLINUX
        -DUSE_LUA

        #-std=c++11
)

#include_directories(../androidlua/lua)
#include_directories(../bz2/include)
include_directories(../../SDL2_image-2.0.5)
include_directories(../../SDL2_ttf-2.0.15)
include_directories(../../SDL2_mixer-2.0.4)
include_directories(${MY_SDL2_HEADER})

#self
#include_directories(Audio)
#include_directories(etc)
#include_directories(File)
#include_directories(Render)
#include_directories(Scripter)

set(MY_LIBS_NAME "LONS")

if(${MY_STATIC_LIBS} MATCHES "#${MY_LIBS_NAME}#")
    set(IS_LIBS_STATIC STATIC)
    message("${MY_LIBS_NAME} will be STATIC")
else()
    set(IS_LIBS_STATIC SHARED)
endif()

set(SRCFILES1
        ./Audio/LOAudioElement.cpp
        ./Audio/LOAudioModule.cpp

        ./etc/BinArray.cpp
        ./etc/LOCodePage.cpp
        ./etc/LOEvent1.cpp
        ./etc/LOIO.cpp
        ./etc/LOLog.cpp
        ./etc/LOString.cpp
        ./etc/LOTimer.cpp
        ./etc/LOVariant.cpp
        ./etc/SDL_mem.cpp

        ./File/LOCompressInfo.cpp
        ./File/LOFileModule.cpp
        ./File/LOPackFile.cpp

        ./Render/LOAction.cpp
        ./Render/LOEffect.cpp
        ./Render/LOFont.cpp
        ./Render/LOImageModule.cpp
        ./Render/LOImageModule2.cpp
        ./Render/LOImageModule_command.cpp
        ./Render/LOLayer.cpp
        ./Render/LOLayerData.cpp
        ./Render/LOMatrix2d.cpp
        ./Render/LOTextDescribe.cpp
        ./Render/LOTexture.cpp

        ./Scripter/FuncInterface.cpp
        ./Scripter/LOScriptPoint.cpp
        ./Scripter/LOScriptReader.cpp
        ./Scripter/LOScriptReader_command.cpp
        ./Scripter/ONSvariable.cpp

        ./SysCore/Android.cpp
        LONS_main.cpp
        )

add_library( # Sets the name of the library.
        ${MY_LIBS_NAME}

        # Sets the library as a shared library.
        ${IS_LIBS_STATIC}
        # Provides a relative path to your source file(s).
        ${SRCFILES1}
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib
        #lua
        #bz2
        XSDL2_image
        XSDL2_mixer
        XSDL2_ttf
        XSDL2
        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        ${MY_LIBS_NAME}
        #lua
        #bz2
        XSDL2_image
        XSDL2_mixer
        XSDL2_ttf
        XSDL2
        ###
        #GLESv1_CM
        GLESv2
        #dl
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})