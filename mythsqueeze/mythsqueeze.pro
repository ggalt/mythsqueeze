include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )

TEMPLATE = lib
CONFIG += plugin \
    thread
TARGET = mythsqueeze
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target
INCLUDEPATH += $${PREFIX}/include/mythtv

# LIBS += -logg \
# -lvorbisfile \
# -lvorbis \
# -lFLAC \
# -lmad \
# -lportaudio
# Input
HEADERS += squeezedefines.h \
    mythsqueeze.h \
    slimdevice.h \
    slimserverinfo.h \
    squeezedisplay.h \
    squeezesettings.h \
    slimcli.h \
    slimimagecache2.h \
    slimdatabasefetch.h \
    squeezepictureflow.h \
    pictureflow.h

SOURCES += main.cpp \
    mythsqueeze.cpp \
    slimdevice.cpp \
    slimserverinfo.cpp \
    squeezedisplay.cpp \
    squeezesettings.cpp \
    slimcli.cpp \
    slimimagecache2.cpp \
    squeezepictureflow.cpp \
    slimdatabasefetch.cpp \
    pictureflow.cpp

QT += network
include ( ../../libs-targetfix.pro )






