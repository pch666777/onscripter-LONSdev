SOURCES += \
    ../LONS/Audio/LOAudioElement.cpp \
    ../LONS/Audio/LOAudioModule.cpp \
    ../LONS/File/LOCompressInfo.cpp \
    ../LONS/File/LOFileModule.cpp \
    ../LONS/File/LOPackFile.cpp \
    ../LONS/LONS_main.cpp \
    ../LONS/Render/LOAction.cpp \
    ../LONS/Render/LOEffect.cpp \
    ../LONS/Render/LOFont.cpp \
    ../LONS/Render/LOImageModule.cpp \
    ../LONS/Render/LOImageModule2.cpp \
    ../LONS/Render/LOImageModule_command.cpp \
    ../LONS/Render/LOLayer.cpp \
    ../LONS/Render/LOLayerData.cpp \
    ../LONS/Render/LOMatrix2d.cpp \
    ../LONS/Render/LOTextDescribe.cpp \
    ../LONS/Render/LOTexture.cpp \
    ../LONS/Scripter/FuncInterface.cpp \
    ../LONS/Scripter/LOCodePage.cpp \
    ../LONS/Scripter/LOScriptPoint.cpp \
    ../LONS/Scripter/LOScriptReader.cpp \
    ../LONS/Scripter/LOScriptReader_command.cpp \
    ../LONS/Scripter/ONSvariable.cpp \
    ../LONS/etc/BinArray.cpp \
    ../LONS/etc/LOCodePage.cpp \
    ../LONS/etc/LOEvent1.cpp \
    ../LONS/etc/LOIO.cpp \
    ../LONS/etc/LOLog.cpp \
    ../LONS/etc/LOString.cpp \
    ../LONS/etc/LOTimer.cpp \
    ../LONS/etc/LOVariant.cpp \
    ../LONS/etc/SDL_mem.cpp

INCLUDEPATH += $$PWD/../../LonsLibs/00_bin/include

INCLUDEPATH += $$PWD/../../LonsLibs/00_bin
DEPENDPATH += $$PWD/../../LonsLibs/00_bin

unix:!macx: LIBS += -L$$PWD/../../LonsLibs/00_bin/ -lSDL2 -ldl -lSDL2_mixer -lvorbis -lvorbisenc -lvorbisfile -logg -ljpeg -lpng -lz -lSDL2_image

unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libSDL2.a
unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libSDL2_mixer.a
unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libvorbis.a
unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libvorbisenc.a
unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libvorbisfile.a
unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libogg.a
unix:!macx: PRE_TARGETDEPS += $$PWD/../../LonsLibs/00_bin/libSDL2_image.a
