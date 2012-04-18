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
    slimCLI = new slimNet( this );
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

    slimCLI->GetData()->SetImageStorage( getImageStoragePath() );      // set the storage path for the image cache

    VERBOSE( VB_GENERAL, "CREATING MythSqueeze2 WINDOW");
    bool foundtheme = false;

    // Load the theme for this screen
    foundtheme = LoadWindowFromXML("MythSqueeze2-ui.xml", "MythSqueeze2", this);

    if (!foundtheme) {
        VERBOSE( VB_IMPORTANT, "no theme" );
        return false;
    }

    // establish the interface
    bool err = false;
    UIUtilE::Assign( this, m_playerName, "PlayerName", &err );
    UIUtilE::Assign( this, m_slimDisplay, "SqueezeDisplay", &err );

    m_disp = new SqueezeDisplay(ui->lblSlimDisplay, this);
    m_disp->Init();

    if( err ) {
        VERBOSE( VB_IMPORTANT, LOC_ERR + "Cannot load screen 'MythSqueeze2'");
        return false;
    }

    BuildFocusList();
    return true;
}

void MythSqueeze::InitPlayer( void )
{
    m_progressDlg = new MythUIProgressDialog("MythSqueeze2 Initializing: ",
                                             m_popupStack, "MSB_m_progressDlg");

    if (m_progressDlg->Create())
    {
        m_popupStack->AddScreen(m_progressDlg, false);
        m_progressDlg->SetMessage( "MythSqueeze2 Initializing: SqueezeSlave Player" );
        progressDlgPos = 0;
        m_progressDlg->SetTotal( 8 );
        connect( slimCLI, SIGNAL(cliInfo(QString)), this, SLOT(slotUpdateProgress(QString)) );
    }
    else
    {
        delete m_progressDlg;
        m_progressDlg = NULL;
    }

    qApp->processEvents();

    DEBUGF( "CREATING SQUEEZESLAVE INSTANCE" );

    connect( squeezePlayer, SIGNAL(readyReadStandardError()), this, SLOT(SqueezePlayerError()) );
    connect( squeezePlayer, SIGNAL(readyReadStandardOutput()), this, SLOT(SqueezePlayerOutput()) );

#ifdef Q_OS_LINUX
    QString program = "squeezeslave-alsa";
#else
    QString program = "squeezeslave";
#endif

    QString macopt = "-m" + QString( MacAddress );
    QString playerdisplaymode = "-l";
    QStringList args ;
    // Note PortAudioDevice is set through "settings", change below for testing if you want to be sure, form is "-o#"
    // PortAudioDevice = "-o0";
#ifdef SQUEEZE_DEBUG
    QString playerdebug = "-dall";
    QString playerdebugfile = "-Y$HOME/slimdebugoutput.txt";
#else
    QString playerdebug = "";
    QString playerdebugfile = "";
#endif
#ifdef SQUEEZE_DEBUG
    args = QStringList() << macopt << playerdisplaymode << PortAudioDevice << playerdebug << playerdebugfile << SlimServerAddr;
#else
    if( PortAudioDevice != "" )
        args = QStringList() << macopt << playerdisplaymode << PortAudioDevice << SlimServerAddr;
    else
        args = QStringList() << macopt << playerdisplaymode << SlimServerAddr;
#endif
    DEBUGF( "player command " << program << " " << args );

    QTime progstart;
    progstart.start();

    squeezePlayer->start( program, args );

    // the short answer is that QProcess::state() doesn't always return "running" under Linux, even when the app is running
    // however, we need to have it running in order for it to be picked up as a "device" later by the CLI interface
    // so we give a bit of a delay (2 seconds) and allow for an earlier exit if we find that state() has returned something useful
    while( squeezePlayer->state() != QProcess::NotRunning ) {
        if( progstart.elapsed() >= 2000 )
            break;
    }

#ifdef Q_OS_WIN32   // this doesn't appear to work under Linux
    if( squeezePlayer->state() == QProcess::NotRunning ) {
        DEBUGF( "Squeezeslave did not start.  Current state is: " << squeezePlayer->state() << " and elapsed time is: " << progstart.elapsed() );
        DEBUGF( "Error message (if any): " << squeezePlayer->errorString() );
        return false;
    }
#endif
    DEBUGF( "Player started, connecting sockets and slots");
    VERBOSE( VB_GENERAL, LOC +  "Player started, connecting sockets and slots");

    // initialize the CLI interface.  Make sure that you've set the appropriate server address and port
    slimCLI->SetMACAddress( QString( MacAddress ) );
    slimCLI->SetSlimServerAddress( SlimServerAddr );
    slimCLI->SetCliPort( QString( gContext->GetSetting( "SqueezeCenterCliPort" ) ) );
    slimCLI->SetCliUsername( QString( gContext->GetSetting( "SqueezeCenterCliUsername" ) ) );
    slimCLI->SetCliPassword( QString( gContext->GetSetting("SqueezeCenterCliPassword") ) );
    slimCLI->SetCliMaxRequestSize( QString( gContext->GetSetting( "SqueezeCenterCliMaxRequestSize" ) ).toAscii() );
    slimCLI->SetCliTimeOutPeriod( QString( gContext->GetSetting( "SqueezeCenterCliTimeout" ) ) );


    connect( slimCLI, SIGNAL(cliError(QString)),
             this, SLOT(slotSystemErrorMsg(QString)) );
    connect( slimCLI, SIGNAL(cliInfo(QString)),
             this, SLOT(slotSystemInfoMsg(QString)) );


    SetUpDisplay();

    slimCLI->Init();  // initialize the CLI/HTTP

    connect( slimCLI->GetData(), SIGNAL(GotMusicData()),
             this, SLOT(slotDatabaseReady()));
    connect( slimCLI->GetData(), SIGNAL(GotImages()),
             this, SLOT(slotImagesReady()));

    // set up connection between interface buttons and the slimserver
}

void MythSqueeze::slotDatabaseReady( void )
{
    isDataReady = true;

    // initialize the devices now so that we are sure Squeezebox server has had pleanty of time to figure out we're connected as a player
    slimCLI->InitDevices();

    if( isImagesReady ) { // if true, then we have both data and images, so create the selection coverflows
        slotSetActivePlayer();
    }
}

void MythSqueeze::slotImagesReady( void )
{
    isImagesReady = true;
    if( isDataReady ) { // if true, then we have both data and images, so create the selection coverflows
        slotSetActivePlayer();
    }
}

void MythSqueeze::CreateBusyDialog(QString title)
{
    if (m_busyDlg)
        return;

    QString message = title;

    m_busyDlg = new MythUIBusyDialog(message, m_popupStack,
                                     "mythsqueezebusydialog");

    if (m_busyDlg->Create())
        m_popupStack->AddScreen(m_busyDlg);
}

void MythSqueeze::SetUpDisplay()
{
    // set up coverflow widget
    DEBUGF( "SET UP DISPLAY AND COVERFLOW");

    flowRect  = m_coverFlowImage->GetArea().toQRect();

    coverFlowWidget = new QWidget( 0 );
    coverFlowWidget->setGeometry( flowRect );
    CoverFlow = new SqueezeCoverFlow( coverFlowWidget );
    CoverFlow->setMinimumSize( flowRect.width(), flowRect.height() );
    connect( CoverFlow, SIGNAL(ImageUpdate()), this, SLOT(slotPaintCoverFlow()));

    // set up slim display area
    displayRect = m_slimDisplay->GetArea().toQRect();
    displayImage = new QImage( displayRect.width(), displayRect.height(), QImage::Format_ARGB32 );
    QColor cBlack;
    cBlack.setRgb( 255, 255, 255 );
    displayImage->fill( cBlack.black() );

    // create a MythImage to use in displaying the slimdisplay
    MythImage *m_DisplayImage = GetMythPainter()->GetFormatImage();
    m_DisplayImage->Assign( *displayImage );
    m_slimDisplay->SetImage( m_DisplayImage );


    cyanGeneral = QColor( Qt::cyan );
    cyanLine1 = QColor( Qt::cyan );

    small.setFamily( "Helvetica" );
    small.setPixelSize( 4 );
    medium.setFamily( "Helvetica" );
    medium.setPixelSize( 4 );
    large.setFamily( "Helvetica" );
    large.setPixelSize( 4 );

    int h = displayRect.height();
    for( int i = 5; QFontInfo( small ).pixelSize() < h/4; i++ )
        small.setPixelSize( i );
    for( int i = 5; QFontInfo( medium ).pixelSize() < h/3; i++ )
        medium.setPixelSize( i );
    for( int i = 5; QFontInfo( large ).pixelSize() < h/2; i++ )
        large.setPixelSize( i );

    // establish font metrics for Line1 (used in scrolling display)
    ln1FM = new QFontMetrics( large );

    // set starting points for drawing on Slim Display standard messages that come through the CLI
    pointLine0 = QPoint( displayRect.width()/100, ( displayRect.height() / 10 ) + small.pixelSize() );
    pointLine1 = QPoint( displayRect.width()/100, ( 9 * displayRect.height() ) / 10 );
    pointLine0Right = QPoint( ( 95 * displayRect.width() ) / 100, ( displayRect.height() / 10 ) + small.pixelSize() );
    pointLine1Right = QPoint( ( 95 * displayRect.width() ) / 100, ( 9 * displayRect.height() ) / 10 );
    upperMiddle = QPoint( displayRect.width() / 2, ( displayRect.height() / 2  ) - (  medium.pixelSize() ) / 4 );
    lowerMiddle = QPoint( displayRect.width() / 2, ( displayRect.height() / 2  ) + ( 5 * medium.pixelSize() ) / 4  );

    Line0Rect = QRect( pointLine0.x(), pointLine0.y() - small.pixelSize(),
                       pointLine0Right.x() - pointLine0.x(),
                       pointLine0.y() );
    Line1Rect = QRect( pointLine1.x(), pointLine1.y() - large.pixelSize(),
                       pointLine1Right.x() - pointLine1.x(),
                       displayRect.height() - ( pointLine1.y() - large.pixelSize() ));
    line1Clipping = QRegion( Line1Rect );
    noClipping = QRegion( displayRect );

    vertTransTimer->setFrameRange( 0, Line1Rect.height() );
    horzTransTimer->setFrameRange( 0, displayRect.width() );
    bumpTransTimer->setFrameRange( 0, ln1FM->width( QChar( 'W')) );

    Line1FontWidth = ln1FM->width( QChar( 'W' ) ) / 40;
    if( Line1FontWidth <= 0 ) // avoid too small a figure
        Line1FontWidth = 1;

    timeRect =  QRectF ( ( qreal )0,
                         ( qreal )( displayRect.height() / 10 ) + ( qreal )small.pixelSize()/2,
                         ( qreal )0,
                         ( qreal )( ( 5 * small.pixelSize() ) / 10 ) );
    timeFillRect = QRectF( timeRect.left(), timeRect.top(), timeRect.width(), timeRect.height() );

    volRect = QRectF (  ( qreal )( displayRect.width() / 20 ),
                        ( qreal )pointLine1.y() - ( qreal )large.pixelSize()/2,
                        ( qreal )( displayRect.width() ) * 0.90,
                        ( qreal )( ( 5 * small.pixelSize() ) / 10 ) );
    volFillRect = QRectF( volRect.left(), volRect.top(), volRect.width(), volRect.height() );

    radius = timeRect.height() / 2;
}

void MythSqueeze::slotUpdateScrollOffset( void )
{
    if( scrollState == PAUSE_SCROLL ) {
        scrollTimer.stop();
        scrollState = SCROLL;
        scrollTimer.setInterval( 33 );
        scrollTimer.start();
    }
    else if( scrollState == SCROLL ) {
        ScrollOffset += Line1FontWidth;
        if( ScrollOffset >= ( lineWidth - ( 3 * Line1Rect.width()  / 4 ) ) ) {
            scrollState = FADE_OUT;
            line1Alpha = 0;
        }
    }
    else if( scrollState == FADE_IN ) {
        line1Alpha -= 15;
        if( line1Alpha <= 0 ) {
            line1Alpha = 0;
            ScrollOffset = 0;
            scrollTimer.stop();
            scrollState = PAUSE_SCROLL;
            scrollTimer.setInterval( 5000 );
            scrollTimer.start();
        }
    }
    else if( scrollState == FADE_OUT ) {
        line1Alpha += 7;
        ScrollOffset += Line1FontWidth; // keep scrolling while fading
        if( line1Alpha >= 255 ) {
            line1Alpha = 255;
            ScrollOffset = 0;
            scrollState = FADE_IN;
        }
    }
    PaintTextDisplay();
}

void MythSqueeze::slotUpdateTransition( int frame )
{
    switch( transitionDirection ) {
    case transLEFT:
        xOffsetOld = 0 - frame;
        xOffsetNew = displayRect.width() - frame;
        yOffsetOld = yOffsetNew = 0;
        break;
    case transRIGHT:
        xOffsetOld = 0 + frame;
        xOffsetNew = frame - displayRect.width();
        yOffsetOld = yOffsetNew = 0;
        break;
    case transUP:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = 0 - frame;
        yOffsetNew = Line1Rect.height() - frame;
        break;
    case transDOWN:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = 0 + frame;
        yOffsetNew = frame - Line1Rect.height();
        break;
    case transNONE:
    default:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = yOffsetNew = 0;
        break;
    }
    PaintTextDisplay();
}

void MythSqueeze::slotTransitionFinished ( void )
{
    vertTransTimer->stop();
    horzTransTimer->stop();
    bumpTransTimer->stop();
    isTransition = false;
    xOffsetOld = xOffsetNew = 0;
    yOffsetOld = yOffsetNew = 0;
    transitionDirection = transNONE;
    slotResetSlimDisplay();
}

void MythSqueeze::StopScroll( void )
{
    scrollTimer.stop();
    ScrollOffset = 0;
    line1Alpha = 0;
    scrollState = PAUSE_SCROLL;
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

void MythSqueeze::slotResetSlimDisplay( void )
{
    StopScroll();
    m_songAlbum->SetText( "Album: " + activeDevice->getCurrentTrack().album );
    m_songArtist->SetText( "Artist: " + activeDevice->getCurrentTrack().artist);
    m_songTitle->SetText( "Title: " + activeDevice->getCurrentTrack().title );
    m_songDuration->SetText( "Duration: " + activeDevice->getCurrentTrack().t_duration.toString( "mm:ss") );
    m_songYear->SetText( "Year: " + activeDevice->getCurrentTrack().year );
    m_songGenre->SetText( "Genre:" + activeDevice->getCurrentTrack().genre );
    int nextTrackNum = activeDevice->getDevicePlaylistIndex() + 1;
    if( nextTrackNum < activeDevice->getDevicePlaylistCount() )  { // not at end of playlist
        if( activeDevice->getDevicePlayList().count() > 0  ) {
            TrackData nextTrack = activeDevice->getDevicePlayList().at( nextTrackNum );
            m_nextSong->SetText("NEXT SONG: " + nextTrack.title + " - " + nextTrack.artist + " - " + nextTrack.album );
        }
    }
    else
        m_nextSong->SetText( "End of Playlist" );
    m_playMode->SetText( "MODE: " + activeDevice->getDeviceMode().toUpper() );
    m_shuffleMode->SetText( "SHUFFLE: " + activeDevice->getDeviceShuffleModeText() );
    m_RepeatMode->SetText( "REPEAT: " + activeDevice->getDeviceRepeatModeText() );
    m_playerName->SetText( QByteArray::fromPercentEncoding( activeDevice->getDeviceName() ) );

    boundingRect = ln1FM->boundingRect( Line1Rect, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer().line1 );
    lineWidth = ln1FM->width( activeDevice->getDisplayBuffer().line1 );
    if(  lineWidth > Line1Rect.width() ) {
        scrollTextLen = lineWidth - Line1Rect.width();
        scrollTimer.setInterval( 5000 );
        scrollTimer.start();
    }
    else {
        scrollState = NOSCROLL;
    }
}

void MythSqueeze::slotUpdateSlimDisplay( void )
{
    boundingRect = ln1FM->boundingRect( Line1Rect, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer().line1 );
    lineWidth = ln1FM->width( activeDevice->getDisplayBuffer().line1 );
    if(  lineWidth > Line1Rect.width() ) {
        if( scrollState == NOSCROLL ) {
            StopScroll();
            scrollTextLen = lineWidth - Line1Rect.width();
            scrollTimer.setInterval( 5000 );
            scrollTimer.start();
        }
    }
    else {
        ScrollOffset = 0;
        scrollState = NOSCROLL;
        scrollTimer.stop();
    }
    PaintTextDisplay();
}

void MythSqueeze::PaintTextDisplay( void )
{
    if( activeDevice == NULL ){
        DEBUGF( "active device is null" );
        VERBOSE( VB_IMPORTANT, LOC_ERR + "Attempted to paint window with 'NULL' as active device");
        return;
    }
    PaintTextDisplay( activeDevice->getDisplayBuffer() );
}

void MythSqueeze::PaintTextDisplay( DisplayBuffer d )
{
    DEBUGF( "PAINTING DISPLAY");
    int playedCount = 0;
    int totalCount = 1; // this way we never get a divide by zero error
    QString timeText = "";
    QPainter p( displayImage );
    QBrush b( QColor( Qt::black ) );
    cyanGeneral.setAlpha( Brightness );
    cyanLine1.setAlpha( Brightness - line1Alpha );

    QBrush c( cyanGeneral );
    QBrush e( c ); // non-filling brush
    e.setStyle( Qt::NoBrush );
    p.setBackground( b );
    p.setBrush( c );
    p.setPen( cyanGeneral );
    p.setFont( large );
    p.eraseRect( displayImage->rect() );

    // draw Line 0  NOTE: Only transitions left or right, but NOT up and down
    if( d.line0.length() > 0 ) {
        p.setFont( small );
        if( isTransition ) {
            p.drawText( pointLine0.x() + xOffsetOld, pointLine0.y(), transBuffer.line0 );
            p.drawText( pointLine0.x() + xOffsetNew, pointLine0.y(), d.line0 );
        }
        else
            p.drawText( pointLine0.x(), pointLine0.y(), d.line0 );
    }

    // draw Line 1
    if( d.line1.length() > 0 ) {
        if( d.line0.left( 8 ) == "Volume (" ) {   // it's the volume, so emulate a progress bar
            qreal volume = d.line0.mid( 8, d.line0.indexOf( ')' ) - 8 ).toInt();
            volFillRect.setWidth( (qreal)volRect.width() * ( volume / (qreal)100 ) );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( volRect, radius, radius );
            p.setBrush( c );
            if( volume > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( volFillRect, radius, radius );
        }
        else {
            QBrush cLine1( cyanLine1 );
            p.setBrush( cLine1 );
            p.setPen( cyanLine1 );
            p.setFont( large );
            p.setClipRegion( line1Clipping );
            if( isTransition ) {
                p.drawText( pointLine1.x() + xOffsetOld, pointLine1.y() + yOffsetOld, transBuffer.line1);
                p.drawText( pointLine1.x() + xOffsetNew, pointLine1.y() + yOffsetNew, d.line1 );
            } else {
                p.drawText( pointLine1.x() - ScrollOffset, pointLine1.y(), d.line1 );
            }
            p.setClipRegion( noClipping );
            p.setBrush( c );
            p.setPen( cyanGeneral );
        }
    }

    // deal with "overlay0" (the right-hand portion of the display) this can be time (e.g., "-3:08") or number of items (e.g., "(2 of 7)")
    if( d.overlay0.length() > 0 ) {
        if( Slimp3Display( d.overlay0 ) ) {
            QRegExp rx( "\\W(\\w+)\\W");
            //            QRegExp rx( "\037(\\w+)\037" );
            QStringList el;
            int pos = 0;

            while ((pos = rx.indexIn(d.overlay0, pos)) != -1) {
                el << rx.cap(1);
                pos += rx.matchedLength();
            }

            rx.indexIn( d.overlay0 );
            QStringList::iterator it = el.begin();
            while( it != el.end() ) {
                QString s = *it;
                if( s.left( 12 ) == "leftprogress" ) { // first element
                    int inc = s.at( 12 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 14 ) == "middleprogress" ) { // standard element
                    int inc = s.at( 14 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 10 ) == "solidblock" ) { // full block
                    playedCount += 4;
                    totalCount += 4;
                }
                else if( s.left( 13 ) == "rightprogress" ) { // end element
                    int inc = s.at( 13 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                ++it;
            }
            QChar *data = d.overlay0.data();
            for( int i = ( d.overlay0.length() - 8 ); i < d.overlay0.length(); i++ ) {
                if( *( data + i ) == ' ' ) {
                    timeText = d.overlay0.mid( i + 1 );
                }
            }
        }
        else if( d.overlay0.contains( QChar( 8 ) ) ) {
            QChar elapsed = QChar(8);
            QChar remaining = QChar(5);
            QChar *data = d.overlay0.data();
            for( int i = 0; i < d.overlay0.length(); i++, data++ ) {
                if( *data == elapsed ) {
                    playedCount++;
                    totalCount++;
                }
                else if( *data == remaining )
                    totalCount++;
                else if( *data == ' ' ) {
                    timeText = d.overlay0.mid( i + 1 );
                }
            }
        }
        else {
            timeText = d.overlay0;
        }
        p.setFont( small );
        QFontMetrics fm = p.fontMetrics();
        p.setClipping( false );
        if( isTransition ) {
            p.drawText( ( pointLine0Right.x() + xOffsetNew ) - fm.width(timeText), pointLine0Right.y(), timeText );
        }
        else {
            p.drawText( pointLine0Right.x() - fm.width(timeText), pointLine0Right.y(), timeText );
        }
        if( totalCount > 1 ) {  // make sure we received data on a progress bar, otherwise, don't draw
            timeRect.setLeft( ( qreal )( pointLine0.x() + fm.width( d.line0.toUpper() ) ) );
            timeRect.setRight( ( qreal )( pointLine0Right.x() - ( qreal )( 3 * fm.width( timeText ) / 2 ) ) );
            timeFillRect.setLeft( timeRect.left() );
            timeFillRect.setWidth( ( playedCount * timeRect.width() ) / totalCount );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( timeRect, radius, radius );
            p.setBrush( c );
            if( playedCount > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( timeFillRect, radius, radius );
        }
    }

    // deal with "overlay1" (the right-hand portion of the display)
    /*
    if( d.overlay1.length() > 0 ) {
        DEBUGF( "Don't know what to do with overlay1 yet" );
    }
*/
    // if we've received a "center" display, it means we're powered down, so draw them
    if( d.center0.length() > 0 ) {
        p.setFont( medium );
        QFontMetrics fm = p.fontMetrics();
        QPoint start = QPoint( upperMiddle.x() - ( fm.width( d.center0 )/2 ), upperMiddle.y() );
        p.drawText( start, d.center0 );
    }

    if( d.center1.length() > 0 ) {
        p.setFont( medium );
        QFontMetrics fm = p.fontMetrics();
        QPoint start = QPoint( lowerMiddle.x() - ( fm.width( d.center1 )/2 ), lowerMiddle.y() );
        p.drawText( start, d.center1 );
    }
    MythImage *m_DisplayImage = GetMythPainter()->GetFormatImage();
    m_DisplayImage->Assign( *displayImage );
    m_slimDisplay->SetImage( m_DisplayImage );
    DEBUGF("END OF PAINT");
}

void MythSqueeze::slotUpdateCoverFlow( int trackIndex )
{
    int currSlide = CoverFlow->centerIndex();
    DEBUGF( "UPDATE COVERFLOW TO INDEX: " << trackIndex );
    if( abs( trackIndex - currSlide ) > 4 ) {
        if( trackIndex > currSlide ) {
            CoverFlow->showSlide( currSlide + 2 );
            CoverFlow->setCenterIndex( trackIndex - 2 );
        }
        else {
            CoverFlow->showSlide( currSlide - 2 );
            CoverFlow->setCenterIndex( trackIndex + 2 );
        }
    }
    CoverFlow->showSlide( trackIndex );
}

void MythSqueeze::slotCreateCoverFlow( void )
{
    DEBUGF( "CREATING COVER FLOW");
    if( !slimCLI->GetData()->AreImagesReady() ) // have the images been set up?
        return;
    CoverFlow->clear();
    QListIterator< TrackData > i( activeDevice->getDevicePlayList() );
    while( i.hasNext() ) {
        TrackData j = i.next();
        QString title = QString( j.title + " - Artist: " + j.artist + " - Album: " + j.album );
        CoverFlow->addSlide( slimCLI->GetData()->GetAlbumCover( j.artwork_track_id ).toImage(), title );
        DEBUGF( "Adding artwork for track id: " << j.artwork_track_id );
    }
    int playListIndex = activeDevice->getDevicePlaylistIndex();
    if( playListIndex > 4 )
        CoverFlow->setCenterIndex( playListIndex - 4 );
    CoverFlow->showSlide( playListIndex );
    //QPixmap::grabWidget( CoverFlow );
}

void MythSqueeze::slotPaintCoverFlow( void )
{
    DEBUGF( "PAINTING COVERFLOW");
    MythImage *mimage = GetMythPainter()->GetFormatImage();
    // NOTE: it would be nice to use CoverFlow->returnBuffer() below, but it doesn't seem to update the image.  Not sure why, but it's worth exploring
    mimage->Assign( QPixmap::grabWidget( CoverFlow ) );
    m_coverFlowImage->SetImage(mimage);
}

void MythSqueeze::slotEscape( void )
{
    DEBUGF( "ESCAPING");
    DisplayBuffer d;
    d.line0 = "Exiting MythSqueeze2 at " + QDateTime::currentDateTime().toString();
    d.line1 = "Goodbye";
    isTransition = false;
    PaintTextDisplay( d );
}

void MythSqueeze::slotLeftArrow( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transRIGHT;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    if( activeDevice->getDisplayBuffer().line0 == "Squeezebox Home" ) {  // are we at the "left-most" menu option?
        bumpTransTimer->start();
    }
    else {
        horzTransTimer->start();
    }
    activeDevice->SendDeviceCommand( QString( "button arrow_left\n" ) );
}

void MythSqueeze::slotRightArrow( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transLEFT;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    horzTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_right\n" ) );
}

void MythSqueeze::slotUpArrow( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transDOWN;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    vertTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_up\n" ) );
}

void MythSqueeze::slotDownArrow( void )
{
    StopScroll();
    isTransition = true;
    transitionDirection = transUP;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    vertTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_down\n" ) );
}


void MythSqueeze::slotSetActivePlayer( void )    // convenience function to set the active player as the player with the same MAC as this computer
{
    DEBUGF( "Setting Active Player as the local machine");
    slotSetActivePlayer( slimCLI->GetData()->GetDeviceFromMac( MacAddress.toPercentEncoding().toLower() ) );
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

QString MythSqueeze::getImageStoragePath( void )
{
    QString path = GetConfDir();

    QDir dir(path);
    if (!dir.exists())
        dir.mkdir(path);
    path += "/MythSqueeze2";
    dir.setPath(path);
    if (!dir.exists())
        dir.mkdir(path);
    return path;
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

void MythSqueeze::slotDeviceModeChange( QString newMode )
{
    m_playMode->SetText( "MODE: " + newMode );
}

void MythSqueeze::slotDeviceMuteStatus( bool newStatus )
{

}

void MythSqueeze::slotDeviceShuffleStatus( QString newStatus )
{
    m_shuffleMode->SetText( "SHUFFLE: " + newStatus );
}

void MythSqueeze::slotDeviceRepeatStatus( QString newStatus )
{
    m_RepeatMode->SetText( "REPEAT: " + newStatus );
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


bool MythSqueeze::Slimp3Display( QString txt )
{
    QRegExp rx( "\037" );      // the CLI overlay for the Slimp3 display uses 0x1F (037 octal) to delimit the segments of the counter
    if( rx.indexIn( txt ) != -1 )
        return true;
    else
        return false;
}


void MythSqueeze::slotUpdateProgress( QString msg )
{
    if( m_progressDlg != NULL ) {
        QString fullMsg = "MythSqueeze2 Initializing: " + msg;
        m_progressDlg->SetMessage( fullMsg );
        m_progressDlg->SetProgress( ++progressDlgPos );
        qApp->processEvents();
    }
}

void MythSqueeze::slotPopupStateSection( void )
{
    QString label = tr("Select Action");

    MythDialogBox *m_menuPopup = new MythDialogBox(label, m_popupStack, "MythSqueezeselectactionpopup");
    if (m_menuPopup->Create())
    {
        m_popupStack->AddScreen(m_menuPopup);
        m_menuPopup->SetReturnEvent(this, "selectaction");

        m_menuPopup->AddButton(tr("Select Player to Manage"), SLOT( showPlayerSelectWin() ), true);
        m_menuPopup->AddButton(tr("Select Music by Artist"), SLOT(showArtistSelectWin()), true);
        m_menuPopup->AddButton(tr("Select Music by Album"), SLOT(showAlbumSelectWin()), true);
        // these next two are not yet implemented
        //        m_menuPopup->AddButton(tr("Select Music by Genre"), SLOT(showGenreSelectWin()), true);
        //        m_menuPopup->AddButton(tr("Select Music by Year"), SLOT(showYearSelectWin()), true);
    }
    else
    {
        delete m_menuPopup;
    }

}

void MythSqueeze::showPlayerSelectWin()
{
    QString label = tr("Select Player to Control");

    MythDialogBox *m_menuPopup = new MythDialogBox(label, m_popupStack,
                                                   "MythSqueezeSelectPlayer");


    if (m_menuPopup->Create())
    {
        m_menuPopup->SetReturnEvent(this, "selectplayer");
        QHashIterator< QString, SlimDevice* > i( slimCLI->GetData()->GetDeviceList() );

        m_popupStack->AddScreen(m_menuPopup);

        while( i.hasNext() ) {
            i.next();
            QString deviceName = QByteArray::fromPercentEncoding( i.value()->getDeviceName() );
            m_menuPopup->AddButton( deviceName, QVariant( i.key() ), false );
        }
    }
    else
    {
        delete m_menuPopup;
    }

}

void MythSqueeze::customEvent(QEvent *event)
{
#ifdef MYTHVERSION_22
    if (event->type() == kMythDialogBoxCompletionEventType)
    {
        DialogCompletionEvent *dce =
                dynamic_cast<DialogCompletionEvent*>(event);
#else
    if (event->type() == DialogCompletionEvent::kEventType)
    {
        DialogCompletionEvent *dce = (DialogCompletionEvent*)(event);
#endif
        QString resultid  = dce->GetId();
        QByteArray playerMac = QVariant( dce->GetData() ).toByteArray();

        if (resultid == "selectplayer")
        {
            slotSetActivePlayer( slimCLI->GetData()->GetDeviceFromMac( playerMac ) );
        }
    }
}

void MythSqueeze::showArtistSelectWin()
{
    SqueezeSelectWin *artistSelectWin = new SqueezeSelectWin( m_popupStack, "SelectByArtist", slimCLI );

    if( artistSelectWin->Create() ) {
        artistSelectWin->SetupWindow();
        connect( artistSelectWin, SIGNAL(playAlbum(QString)), this, SLOT(playAlbum(QString)));
        connect( artistSelectWin, SIGNAL(addAlbum(QString)), this, SLOT(addAlbum(QString)));
        m_popupStack->AddScreen( artistSelectWin );
    }
    else {
        delete artistSelectWin;
        VERBOSE( VB_IMPORTANT, "Unable to create selection window" );
    }

}

void MythSqueeze::showAlbumSelectWin( void )
{
    SqueezeSelectWin *albumSelectWin = new SqueezeSelectWin( m_popupStack, "SelectByAlbum", slimCLI );

    if( albumSelectWin->Create() ) {
        albumSelectWin->SetupWindow();
        connect( albumSelectWin, SIGNAL(playAlbum(QString)), this, SLOT(playAlbum(QString)));
        connect( albumSelectWin, SIGNAL(addAlbum(QString)), this, SLOT(addAlbum(QString)));
        m_popupStack->AddScreen( albumSelectWin );
    }
    else {
        delete albumSelectWin;
        VERBOSE( VB_IMPORTANT, "Unable to create selection window" );
    }
}

void MythSqueeze::showGenreSelectWin( void )
{
    SqueezeSelectWin *genreSelectWin = new SqueezeSelectWin( m_popupStack, "SelectByGenre", slimCLI );

    if( genreSelectWin->Create() ) {
        genreSelectWin->SetupWindow();
        connect( genreSelectWin, SIGNAL(playAlbum(QString)), this, SLOT(playAlbum(QString)));
        connect( genreSelectWin, SIGNAL(addAlbum(QString)), this, SLOT(addAlbum(QString)));
        m_popupStack->AddScreen( genreSelectWin );
    }
    else {
        delete genreSelectWin;
        VERBOSE( VB_IMPORTANT, "Unable to create selection window" );
    }
}

void MythSqueeze::showYearSelectWin( void )
{
    SqueezeSelectWin *yearSelectWin = new SqueezeSelectWin( m_popupStack, "SelectByYear", slimCLI );

    if( yearSelectWin->Create() ) {
        yearSelectWin->SetupWindow();
        connect( yearSelectWin, SIGNAL(playAlbum(QString)), this, SLOT(playAlbum(QString)));
        connect( yearSelectWin, SIGNAL(addAlbum(QString)), this, SLOT(addAlbum(QString)));
        m_popupStack->AddScreen( yearSelectWin );
    }
    else {
        delete yearSelectWin;
        VERBOSE( VB_IMPORTANT, "Unable to create selection window" );
    }
}

void MythSqueeze::playAlbum( QString albumID )
{
    // album id to play
    QString fullCmd = "playlistcontrol cmd:load album_id:" + albumID;
    activeDevice->SendDeviceCommand( fullCmd );
}

void MythSqueeze::addAlbum( QString albumID )
{
    // album id to add to current playlist
    QString fullCmd = "playlistcontrol cmd:add album_id:" + albumID;
    activeDevice->SendDeviceCommand( fullCmd );
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

