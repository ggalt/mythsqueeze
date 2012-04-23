#include <mythtv/mythcontext.h>

#include "squeezesettings.h"
#include <QProcess>
#include <QList>
#include <QListIterator>
#include <QByteArray>
#include <QString>
#include <QFile>
#include <QDialog>
#include <QCursor>
#include <QDir>
#include <QImage>

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

static HostComboBox *PortAudioOutputDevice()
{
    // get the list of audio devices available
    QProcess proc;
    QStringList args;
    args.append(QString("-L"));
    proc.start("squeezeslave", args );

    proc.waitForReadyRead(2000);

    QByteArray m_out = proc.readAllStandardOutput();
    QList<QByteArray> outDevs = m_out.split('\n');

    proc.close();

    HostComboBox *gc = new HostComboBox("PortAudioOutputDevice");

    gc->setLabel(QObject::tr("Select Portaudio Device for output"));
    gc->addSelection("##DEFAULT AUDIO DEVICE##");       // dummy device for taking the default audio output
    QListIterator<QByteArray> i(outDevs);
    i.next();   // skip the first line, which is garbage
    while(i.hasNext()) {
        gc->addSelection(QString(i.next()));
    }

    gc->setHelpText(QObject::tr("The default device for PortAudio is often Pulse audio, which gets turned off by MythTV.  Use 'squeezeslave-alsa -L' at command prompt to see possible devices."));
    return gc;
}

SqueezeSettings::SqueezeSettings()
{
    VerticalConfigurationGroup* general = new VerticalConfigurationGroup(false);
    general->setLabel(QObject::tr("MythSqueeze Settings"));
    general->addChild(SqueezeCenterIP());
    general->addChild(SqueezeCenterCliPort());
    general->addChild(SqueezeCenterCliUsername());
    general->addChild(SqueezeCenterCliPassword());
    general->addChild(PortAudioOutputDevice());
    addChild(general);
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
