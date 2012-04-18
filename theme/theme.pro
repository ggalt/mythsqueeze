include ( ../../mythconfig.mak )
include ( ../../settings.pro )

QMAKE_STRIP = echo

TARGET = themenop
TEMPLATE = app
CONFIG -= qt moc

!macx:QMAKE_COPY_DIR = sh ../../cpsvndir

defaultfiles.path = $${PREFIX}/share/mythtv/themes/default
defaultfiles.files = default/*.xml

widefiles.path = $${PREFIX}/share/mythtv/themes/default-wide
widefiles.files = default-wide/*.xml
wideimages.path = $${PREFIX}/share/mythtv/themes/default-wide/msb_images
wideimages.files = default-wide/images/*.png

defaultimages.path = $${PREFIX}/share/mythtv/themes/default/msb_images
defaultimages.files = default/images/*.png

menufiles.path = $${PREFIX}/share/mythtv/
menufiles.files = menus/*.xml

INSTALLS += defaultfiles widefiles menufiles wideimages
INSTALLS += defaultfiles menufiles defaultimages

# Input
SOURCES += ../../themedummy.c
