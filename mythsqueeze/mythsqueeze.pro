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
    squeezesettings.h \
    slimcli.h \
    mythsqueezedisplay.h

SOURCES += main.cpp \
    mythsqueeze.cpp \
    slimdevice.cpp \
    slimserverinfo.cpp \
    squeezesettings.cpp \
    slimcli.cpp \
    mythsqueezedisplay.cpp

QT += network sql xml
include ( ../../libs-targetfix.pro )









