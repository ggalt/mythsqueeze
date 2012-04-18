#include <mythtv/mythcontext.h>

#include "squeezesettings.h"
#include <qfile.h>
#include <qdialog.h>
#include <qcursor.h>
#include <qdir.h>
#include <qimage.h>

// #include "config.h"

// General Settings

static HostLineEdit *SqueezeCenterIP()
{
    HostLineEdit *gc = new HostLineEdit("SqueezeCenterIP");
    gc->setLabel(QObject::tr("SqueezeServer IP Address"));
    gc->setValue("127.0.0.1");
    gc->setHelpText(QObject::tr("This must be a valid IP address or host name"));
    return gc;
}

static HostLineEdit *SqueezeCenterCliPort()
{
    HostLineEdit *gc = new HostLineEdit("SqueezeCenterCliPort");
    gc->setLabel(QObject::tr("SqueezeServer CLI Port"));
    gc->setValue("9090");
    gc->setHelpText(QObject::tr("CLI port is usually 9090 but you may have changed it"));
    return gc;
}

static HostLineEdit *SqueezeCenterCliUsername()
{
    HostLineEdit *gc = new HostLineEdit("SqueezeCenterCliUsername");
    gc->setLabel(QObject::tr("SqueezeServer CLI User Name"));
    gc->setValue("");
    gc->setHelpText(QObject::tr("A username is necessary if you set security on SqueezeServer"));
    return gc;
}

static HostLineEdit *SqueezeCenterCliPassword()
{
    HostLineEdit *gc = new HostLineEdit("SqueezeCenterCliPassword");
    gc->setLabel(QObject::tr("SqueezeServer CLI Password"));
    gc->setValue("");
    gc->setHelpText(QObject::tr("A password is necessary if you set security on SqueezeServer"));
    return gc;
}

static HostLineEdit *SqueezeCenterCliTimeout()
{
    HostLineEdit *gc = new HostLineEdit("SqueezeCenterCliTimeout");
    gc->setLabel(QObject::tr("Enter timeout period for CLI requests (in milliseconds)"));
    gc->setValue("5000");
    gc->setHelpText(QObject::tr("Number of milliseconds before CLI timeout -- increase if requests are failing"));
    return gc;
}

static HostLineEdit *SqueezeCenterCliMaxRequestSize()
{
    HostLineEdit *gc = new HostLineEdit("SqueezeCenterCliMaxRequestSize");
    gc->setLabel(QObject::tr("Enter max number of items for CLI to return for each request"));
    gc->setValue("20");        // NOTE: QT's QTcpsocket seems to bork on receiving data requests from CLI larger that 16384 bytes.  Requesting about 20 items at a time seems about right.
    gc->setHelpText(QObject::tr("Maximum number of items requested by CLI in any single request -- may fail if setting is larger than 50"));
    return gc;
}

static HostLineEdit *PortAudioOutputDevice()
{
    HostLineEdit *gc = new HostLineEdit("PortAudioOutputDevice");
    gc->setLabel(QObject::tr("Enter Correct Device # for PortAudio To Use (leave blank for 'Default')"));
    gc->setValue("");        // NOTE: squeezeslave-alse -L will list ports, it would be nice to list port available here
    gc->setHelpText(QObject::tr("The default device for PortAudio is often Pulse audio, which gets turned off by MythTV.  Use 'squeezeslave-alsa -L' at command prompt to see possible devices."));
    return gc;
}

SqueezeSettings::SqueezeSettings()
{
    VerticalConfigurationGroup* general = new VerticalConfigurationGroup(false);
    general->setLabel(QObject::tr("MythSqueezeBox Settings"));
    general->addChild(SqueezeCenterIP());
    general->addChild(SqueezeCenterCliPort());
    general->addChild(SqueezeCenterCliUsername());
    general->addChild(SqueezeCenterCliPassword());
    general->addChild(SqueezeCenterCliTimeout());
    general->addChild(SqueezeCenterCliMaxRequestSize());
    general->addChild(PortAudioOutputDevice());
    addChild(general);
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
