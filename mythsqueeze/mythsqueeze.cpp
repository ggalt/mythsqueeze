#include "mythsqueeze.h"


/**
 *  \param parent Pointer to the screen stack
 *  \param name The name of the window
 */
MythSqueeze::MythSqueeze(MythScreenStack *parent, const char *name)
    : MythScreenType (parent, name)
{
    // want to have this created here so we can set the SlimServer IP and MAC addresses before we init()
    squeezePlayer = new QProcess( this );
    slimCLI = new SlimCLI( this );
    activeDevice = NULL;
    PortAudioDevice = "";
    isStartUp = true;

    activeDevice = NULL;
    isStartUp = true;

    connect( slimCLI, SIGNAL(cliInfo(QString)), waitWindow, SLOT(showMessage(QString)) );

    m_popupStack = GetMythMainWindow()->GetStack("popup stack");
}

MythSqueeze::~MythSqueeze()
{
    VERBOSE( VB_GENERAL, LOC + "Exiting MythSqueeze Plugin at " + QDateTime::currentDateTime().toString() );
    squeezePlayer->close();
}

bool MythSqueeze::Create()
{
    // SET UP SERVER AND PLAYER INFO
    getSqueezeCenterAddress();
    getplayerMACAddress();
    getPortAudioDevice();

    VERBOSE( VB_GENERAL, "CREATING MythSqueeze2 WINDOW");
    bool foundtheme = false;

    // Load the theme for this screen
    foundtheme = LoadWindowFromXML("MythSqueeze-ui.xml", "MythSqueeze", this);

    if (!foundtheme) {
        VERBOSE( VB_IMPORTANT, "no theme" );
        return false;
    }

    // establish the interface
    bool err = false;
    UIUtilE::Assign( this, m_playerName, "PlayerName", &err );
    UIUtilE::Assign( this, m_squeezeDisplay, "SqueezeDisplay", &err );

    m_disp = new MythSqueezeDisplay(m_squeezeDisplay, this);

    if( err ) {
        VERBOSE( VB_IMPORTANT, LOC_ERR + "Cannot load screen 'MythSqueeze2'");
        return false;
    }

    BuildFocusList();
    return true;
}

void MythSqueeze::InitPlayer( void )
{
    connect( squeezePlayer, SIGNAL(readyReadStandardError()), this, SLOT(SqueezePlayerError()) );
    connect( squeezePlayer, SIGNAL(readyReadStandardOutput()), this, SLOT(SqueezePlayerOutput()) );

    // SETUP SQUEEZESLAVE PROCESS

#ifdef Q_OS_LINUX
    QString program = "squeezeslave";
    QString playeropt = "-l";
#else
    QString program = "squeezeslave";
    QString playeropt = "-D";
#endif

    QStringList args;

    args.append(QString("-m" + QString( MacAddress )));
    args.append(playeropt);
    args.append(QString("-P" + SlimServerAudioPort));
    if( PortAudioDevice.length() > 0 ) {  // NOTE: list contains the number, name and a description.  We need just the number
        PortAudioDevice = PortAudioDevice.left(PortAudioDevice.indexOf(":")).trimmed();
        args.append(QString("-o"+PortAudioDevice));
    }
    args.append(SlimServerAddr);
    DEBUGF( "player command " << program << " " << args );

    progstart.start();

    squeezePlayer->start( program, args );
    // the short answer is that QProcess::state() doesn't always return "running" under Linux, even when the app is running
    // however, we need to have it running in order for it to be picked up as a "device" later by the CLI interface
    // so we give a bit of a delay (2 seconds) and allow for an earlier exit if we find that state() has returned something useful
#ifdef Q_OS_WIN32   // this doesn't appear to work under Linux
    if( squeezePlayer->state() == QProcess::NotRunning ) {
        DEBUGF( "Squeezeslave did not start.  Current state is: " << squeezePlayer->state() << " and elapsed time is: " << progstart.elapsed() );
        DEBUGF( "Error message (if any): " << squeezePlayer->errorString() );
        return false;
    }
#else
    while( squeezePlayer->state() != QProcess::NotRunning ) {
        if( progstart.elapsed() >= 2000 )
            break;
    }
#endif

    // initialize the CLI interface.  Make sure that you've set the appropriate server address and port
    slimCLI->SetMACAddress( QString( MacAddress ) );
    slimCLI->SetSlimServerAddress( SlimServerAddr );
    slimCLI->SetCliPort( QString( gContext->GetSetting( "SqueezeCenterCliPort" ) ) );
    slimCLI->SetCliUsername( QString( gContext->GetSetting( "SqueezeCenterCliUsername" ) ) );
    slimCLI->SetCliPassword( QString( gContext->GetSetting("SqueezeCenterCliPassword") ) );

    connect( slimCLI, SIGNAL(cliError(QString)),
             this, SLOT(slotSystemErrorMsg(QString)) );
    connect( slimCLI, SIGNAL(cliInfo(QString)),
             this, SLOT(slotSystemInfoMsg(QString)) );

    m_disp->Init(); // set up display

    slimCLI->Init();

    connect( slimCLI->GetData(), SIGNAL(GotMusicData()),
             this, SLOT(slotDatabaseReady()));
    connect( slimCLI->GetData(), SIGNAL(GotImages()),
             this, SLOT(slotImagesReady()));

    // set up connection between interface buttons and the slimserver
}

void MythSqueeze::SendPlayerMessage( QString msg, QString duration, bool includeTime )
{
    QString cmd = "display ";
    if( includeTime ) {
        QDateTime t;
        QByteArray baDate = t.currentDateTime().toString(QString("ddd MMMM dd yyyy -- hh:mm AP")).toAscii();
        QString dt = QString( baDate.toPercentEncoding() ) + " ";
        cmd += dt;
    }
    cmd += QString( msg.toAscii().toPercentEncoding() ) + " ";
    cmd += duration;
    DEBUGF( "Sending Player Message: " << msg << " at " << QDateTime::currentDateTime().toString());
    activeDevice->SendDeviceCommand( cmd );
}

void MythSqueeze::slotEscape( void )
{
    DEBUGF( "ESCAPING");
//    m_disp->
//    DisplayBuffer d;
//    d.line0 = "Exiting MythSqueeze at " + QDateTime::currentDateTime().toString();
//    d.line1 = "Goodbye";
//    isTransition = false;
}

void MythSqueeze::slotPlay( void )
{
    DEBUGF("");
        activeDevice->SendDeviceCommand( QString( "button play.single\n" ) );
}

void MythSqueeze::slotAdd( void )
{
    DEBUGF("");
        activeDevice->SendDeviceCommand( QString( "button add.single\n" ) );
}

void MythSqueeze::slotLeftArrow( void )
{
    DEBUGF("");
        m_disp->LeftArrowEffect();
        activeDevice->SendDeviceCommand( QString( "button arrow_left\n" ) );
}

void MythSqueeze::slotRightArrow( void )
{
    DEBUGF("");
        m_disp->RightArrowEffect();
        activeDevice->SendDeviceCommand( QString( "button arrow_right\n" ) );
}

void MythSqueeze::slotUpArrow( void )
{
    DEBUGF("");
    m_disp->UpArrowEffect();
    activeDevice->SendDeviceCommand( QString( "button arrow_up\n" ) );
}

void MythSqueeze::slotDownArrow( void )
{
    DEBUGF("");
    m_disp->DownArrowEffect();
    activeDevice->SendDeviceCommand( QString( "button arrow_down\n" ) );
}

void MythSqueeze::slotSetActivePlayer( void )    // convenience function to set the active player as the player with the same MAC as this computer
{
    DEBUGF( "Setting Active Player as the local machine");
    slotSetActivePlayer( MacAddress.toPercentEncoding().toLower());
    DEBUGF( "ENABLING PLAYER");
    slotEnablePlayer();
    DEBUGF( "PLAYER ENABLED");
}

void MythSqueeze::slotSetActivePlayer( SlimDevice *d )
{
    if( activeDevice != NULL ) {    // first, disconnect current player, but only if there is a current player
        disconnect( activeDevice, SIGNAL(NewSong()),
                    this, SLOT(slotResetSlimDisplay()) );
        disconnect( activeDevice, SIGNAL(ModeChange(QString)),
                    this, SLOT(slotDeviceModeChange(QString)));
        disconnect( activeDevice, SIGNAL(Mute(bool)),
                    this, SLOT(slotDeviceMuteStatus(bool)));
        disconnect( activeDevice, SIGNAL(RepeatStatusChange(QString)),
                    this, SLOT(slotDeviceRepeatStatus(QString)));
        disconnect( activeDevice, SIGNAL(ShuffleStatusChange(QString)),
                    this, SLOT(slotDeviceShuffleStatus(QString)));
        disconnect( activeDevice, SIGNAL(SlimDisplayUpdate()),
                    this, SLOT(slotUpdateSlimDisplay()) );
        disconnect( activeDevice, SIGNAL(CoverFlowUpdate( int )),
                    this, SLOT(slotUpdateCoverFlow(int)) );
        disconnect( activeDevice, SIGNAL(CoverFlowCreate()),
                    this, SLOT(slotCreateCoverFlow()) );
    }
    QString logMsg = "Changing Active Player to player with Mac address of : " + d->getDeviceMAC() + " with name of :" + d->getDeviceName();
    DEBUGF( logMsg );
    VERBOSE( VB_GENERAL, LOC + logMsg );
    slimCLI->GetData()->SetCurrentDevice( d );
    activeDevice = d;
    slotResetSlimDisplay();
    slotCreateCoverFlow();
    connect( activeDevice, SIGNAL(NewSong()),
             this, SLOT(slotResetSlimDisplay()) );
    connect( activeDevice, SIGNAL(ModeChange(QString)),
             this, SLOT(slotDeviceModeChange(QString)));
    connect( activeDevice, SIGNAL(Mute(bool)),
             this, SLOT(slotDeviceMuteStatus(bool)));
    connect( activeDevice, SIGNAL(RepeatStatusChange(QString)),
             this, SLOT(slotDeviceRepeatStatus(QString)));
    connect( activeDevice, SIGNAL(ShuffleStatusChange(QString)),
             this, SLOT(slotDeviceShuffleStatus(QString)));
    connect( activeDevice, SIGNAL(SlimDisplayUpdate()),
             this, SLOT(slotUpdateSlimDisplay()) );
    connect( activeDevice, SIGNAL(CoverFlowUpdate( int )),
             this, SLOT(slotUpdateCoverFlow(int)) );
    connect( activeDevice, SIGNAL(CoverFlowCreate()),
             this, SLOT(slotCreateCoverFlow()) );
}

void MythSqueeze::getplayerMACAddress( void )
{
    MacAddress = QByteArray( "00:00:00:00:00:04" );

    QList<QNetworkInterface> netList = QNetworkInterface::allInterfaces();
    QListIterator<QNetworkInterface> i( netList );

    QNetworkInterface t;

    while( i.hasNext() ) {  // note: this grabs the first functional, non-loopback address there is.  It may not the be interface on which you really connect to the slimserver
        t = i.next();
        if( !t.flags().testFlag( QNetworkInterface::IsLoopBack ) &&
            t.flags().testFlag( QNetworkInterface::IsUp ) &&
            t.flags().testFlag( QNetworkInterface::IsRunning ) ) {
            MacAddress = t.hardwareAddress().toAscii().toLower();
            return;
        }
    }
}

void MythSqueeze::getSqueezeCenterAddress( void )
{
    SlimServerAddr = QString( gContext->GetSetting("SqueezeCenterIP") );
}

void MythSqueeze::getPortAudioDevice( void )
{
    PortAudioDevice = QString( gContext->GetSetting("PortAudioOutputDevice")).trimmed();
    if( PortAudioDevice != "" ) {
        PortAudioDevice = "-o" + PortAudioDevice.trimmed();
        DEBUGF( "PortAudioDevice = " << PortAudioDevice );
    }
}

void MythSqueeze::slotDisablePlayer( void )
{
    CreateBusyDialog( "Player Busy . . . Please Wait");
}

void MythSqueeze::slotEnablePlayer( void )
{
    if( m_progressDlg != NULL ) {
        disconnect( slimCLI, SIGNAL(cliInfo(QString)), this, SLOT(slotUpdateProgress(QString)) );
        m_progressDlg->Close();
        m_progressDlg = NULL;
    }

    SendPlayerMessage( "Welcome To MythSqueeze2", "5", true );
}

void MythSqueeze::SqueezePlayerError( void )
{
    QString errMsg = LOC_ERR + squeezePlayer->readAllStandardError();
    DEBUGF( errMsg );
    VERBOSE( VB_IMPORTANT, errMsg );
}

void MythSqueeze::SqueezePlayerOutput( void )
{
    QString errMsg = LOC + squeezePlayer->readAllStandardOutput();
    DEBUGF( errMsg );
    VERBOSE( VB_GENERAL, errMsg );
}

void MythSqueeze::slotSystemInfoMsg( QString msg )
{
    DEBUGF( msg );
    VERBOSE( VB_GENERAL, LOC + msg );
}

void MythSqueeze::slotSystemErrorMsg( QString err )
{
    DEBUGF( err );
    VERBOSE( VB_IMPORTANT, LOC_ERR + err );
    Close();    // exit program
}

bool MythSqueeze::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;
    QStringList actions;
    gContext->GetMainWindow()->TranslateKeyPress("MythSB", event, actions);

    for (int i = 0; i < actions.size() && !handled; i++)
    {
        QString action = actions[i];
        handled = true;
        DEBUGF( "Button action: " << action );
        if (action == 	"MENU") { // SWITCHES TO GIVING FULL VFD CONTROL
            slotPopupStateSection();
        }
        else if (action == 	"DOWNARROW")
            slotDownArrow();
        else if (action == 	"UPARROW")
            slotUpArrow();
        else if (action == 	"LEFTARROW")
            slotLeftArrow();
        else if (action == 	"RIGHTARROW")
            slotRightArrow();
        else if (action == 	"SHUFFLE")
            slotShuffle();
        else if (action == 	"REPEAT")
            slotRepeat();
        else if (action == 	"NOWPLAYING")
            slotNowPlaying();
        else if (action == 	"ADD")
            slotAdd();
        else if (action == 	"REWIND")
            slotRewind();
        else if (action == 	"FASTFORWARD")
            slotFForward();
        else if (action == 	"STOP")
            slotStop();
        else if (action == 	"PLAY")
            slotPlay();
        else if (action == 	"PAUSE")
            slotPause();
        else if (action == 	"MUTE")
            slotMute();
        else if (action == 	"VOLUP")
            slotVolUp();
        else if (action == 	"VOLDOWN")
            slotVolDown();
        else if (action == 	"BTN0")
            slot0PAD();
        else if (action == 	"BTN1")
            slot1PAD();
        else if (action == 	"BTN2")
            slot2PAD();
        else if (action == 	"BTN3")
            slot3PAD();
        else if (action == 	"BTN4")
            slot4PAD();
        else if (action == 	"BTN5")
            slot5PAD();
        else if (action == 	"BTN6")
            slot6PAD();
        else if (action == 	"BTN7")
            slot7PAD();
        else if (action == 	"BTN8")
            slot8PAD();
        else if (action == 	"BTN9")
            slot9PAD();
        else if (action == 	"SLEEP")
            slotSleep();
        else if (action == 	"SIZE")
            slotSize();
        else if (action == 	"SEARCH")
            slotSearch();
        else if (action == 	"BRIGHT")
            slotBright();
        else if (action ==      "ESCAPE") {
            slotEscape();
            handled = false;
        }
        else
            handled = false;
    }

    if (!handled) {
        handled = MythScreenType::keyPressEvent( event );
    }
    return handled;
}

void MythSqueeze::KeyActivity( int key )
{
    emit KeyPress( key );
}

void MythSqueeze::ButtonActivity( QString button )
{
    emit ButtonPress( button );
}



/* vim: set expandtab tabstop=4 shiftwidth=4: */

