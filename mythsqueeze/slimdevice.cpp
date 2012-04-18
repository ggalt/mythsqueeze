#include "slimdevice.h"
#include "slimcli.h"

#ifdef SLIMDEVICE_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SlimDevice::SlimDevice( QByteArray MAC, QByteArray Name, QByteArray IP, QByteArray Number, QObject *parent, const char *name )
    : QObject( parent )
{
    setDeviceMAC( MAC );
    setDeviceName( Name );
    setDeviceIP( IP );
    setDeviceNumber( Number );
    response.resize( 100 );
    deviceConnected = false; // is device connected (always true if device is a Slimp3)
    devicePower = false; // is device on or off?
    deviceMute = false; // is device muted
    devicePlaylistCount = 0; // number of tracks in current playlist
    devicePlaylistIndex = 0;  // where are we in the current playlist
    doneGettingDeviceSettings = false;
    initializingDeviceSettings = true;
}

SlimDevice::SlimDevice( QObject *parent, const char *name )
    : QObject( parent )
{
    deviceConnected = false; // is device connected (always true if device is a Slimp3)
    devicePower = false; // is device on or off?
    deviceMute = false; // is device muted
    devicePlaylistCount = 0; // number of tracks in current playlist
    devicePlaylistIndex = 0;  // where are we in the current playlist
    doneGettingDeviceSettings = false;
    initializingDeviceSettings = true;
    myDisplay.displayUpdateType = "";
    myDisplay.line0 = "";
    myDisplay.line1 = "";
    myDisplay.overlay0 = "";
    myDisplay.overlay1 = "";

}

SlimDevice::~SlimDevice( void )
{
    DEBUGF( "SlimDevice killed: " << this->deviceMAC );
}

void SlimDevice::Init( SlimCLI *cli )
{
    // OK, let's start the process of getting information on ourselves
    // first start initalize the player timer so we will always know how long we've been playin
    playerTime.start();

    // first let's set up access to the CLI, so we can communicate and get info about the system
    cliInterface = cli;
    doneGettingDeviceSettings = false;

    // finally, let's send the command to get the information this device
    // this command gets genre, artist, album, title, year, duration, artwork_id (to get cover image)
    //  SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,J \n" ) );
    // NOTE: "e" should deliver album_id
    SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,c \n" ) );
}

/*
  * This is a convenience function that automatically prepends the MAC address of this
  * device onto a command and sends it via the CLI
*/
void SlimDevice::SendDeviceCommand( QString cmd )
{
    DEBUGF("SENDING COMMAND:" << cmd)
    if( cmd.at( 0 ) != ' ' )  // if there is no leading space, add one
        cmd = QString( " " ) + cmd;
    cmd = getDeviceMAC() + cmd;
    DEBUGF( "Sending Device command: " << cmd );
    cliInterface->SendCommand( cmd.toAscii() );    // send command through cli, noting that we don't need to add MAC address at cli
}

void SlimDevice::ProcessStatusSetupMessage( QByteArray msg )
{
    DEBUGF( "StatusSetupMessage: " << msg );
    /*
   * we can't yet remove the percent encoding because some of the msgs like
   * "playlist index" have a space (%20) in them and it will throw off the sectioning of the string
   * using mgs.split below if we do it now, but we do need to convert the %3A to a ':'
  */
    devicePlayList.clear();
    msg.replace( "%3A", ":" );
    QList<QByteArray> MsgList = msg.split( ' ' );    // put all of the status messages into an array for processing
    bool playlistInfo = false;

    QListIterator<QByteArray> i( MsgList );
    DEBUGF( "list has size: " << MsgList.count() );
    for( int c = 0; c < 4 && i.hasNext(); c++ ) // note we start on the 4th field because the first 4 are <"status"> <"0"> <"1000"> <"tags:">
        i.next();
    int t = 0;
    while( i.hasNext() ) {
        t++;
        QString s = QString( i.next() );
        DEBUGF( "List item [ " << t-1 << " ] is: " << s );
        if( !playlistInfo ) {
            if( s.section( ':', 0, 0 ) == "player_connected" )
                deviceConnected = ( s.section( ':', 1, 1 ) == "1" );
            else if( s.section( ':', 0, 0 ) == "power" )
                devicePower = ( s.section( ':', 1, 1 ) == "1" );
            else if( s.section( ':', 0, 0 ) == "mode" )
                this->deviceMode = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "time" )
                this->deviceCurrentSongTime = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "rate" )
                this->deviceRate = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "duration" )
                this->deviceCurrentSongDuration = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "mixer%20volume" )
                this->deviceVol = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "playlist%20repeat" )
                this->deviceRepeatMode = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "playlist%20shuffle" )
                this->deviceShuffleMode = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "playlist_cur_index" )
                devicePlaylistIndex = s.section( ':', 1, 1 ).toInt();
            else if( s.section( ':', 0, 0 ) == "playlist_tracks" ) {  // OK, we've gotten to the portion of the program where the playlist info is ready
                devicePlaylistCount = s.section( ':', 1, 1 ).toInt();
                playlistInfo = true;
            }
        }
        else {
            if( s.section( ':', 0, 0 ) == "playlist%20index" ) {
                this->devicePlayList.append( TrackData() );
                devicePlayList.last().playlist_index = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            }
            else if( s.section( ':', 0, 0 ) == "title" )
                devicePlayList.last().title = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "album_id")
                devicePlayList.last().album_id = QByteArray::fromPercentEncoding(s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "genre" )
                devicePlayList.last().genre = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "artist" )
                devicePlayList.last().artist = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "album" )
                devicePlayList.last().album = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "tracknum" )
                devicePlayList.last().tracknum = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "year" )
                devicePlayList.last().year = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "duration" )
                devicePlayList.last().duration = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
            else if( s.section( ':', 0, 0 ) == "coverid" )
                devicePlayList.last().coverid = QByteArray::fromPercentEncoding( s.section( ':', 1, 1 ).toAscii() );
        }
    }
    doneGettingDeviceSettings = true;   // set this so that when the CLI checks, we can say we are done
    SendDeviceCommand( QString( "mode ? \n" ) );
    emit playlistCoverFlowCreate();
    emit DeviceReady();
    // note: we still need to get mute and sync
}

void SlimDevice::ProcessDisplayStatusMsg( QByteArray Response )
{
    response = Response;
    // NOTE: this assumes that the MAC address of the player has been stripped off
    DEBUGF( "DISPLAY MODE message received: " << response );
    response.replace( "\n", "" ); // make sure there are no line feeds!!
    response = response.trimmed();

    //NOTE: Response still in escape/percent encoding, so use blank spaces to create fields
    QList< QByteArray >fields = response.split( ' ' );
    if( fields.size() > 2 ) {   // this avoids trying to parse the response to the setup command
        // zero the display fields so that we don't display old data for fields that get no update
        myDisplay.displayUpdateType = "";
        myDisplay.line0 = "";
        myDisplay.line1 = "";
        myDisplay.overlay0 = "";
        myDisplay.overlay1 = "";
        myDisplay.center0 = "";
        myDisplay.center1 = "";
        DEBUGF( "Clearning old lines" );

        for( int t = 0; t < fields.size(); t++ ) {
            QString field = QByteArray::fromPercentEncoding(fields.at( t ) );
            DEBUGF( "Field " << t << " is " << field );
            if( field.section( ':', 0, 0 ) == "type" )
                myDisplay.displayUpdateType = field.section( ':', 1 );
            else if( field.section( ':', 0, 0 ) == "line0" )
                myDisplay.line0 = field.section( ':', 1 );
            else if( field.section( ':', 0, 0 ) == "line1" )
                myDisplay.line1 = field.section( ':', 1 );
            else if( field.section( ':', 0, 0 ) == "overlay0" )
                myDisplay.overlay0 = field.section( ':', 1 );
            else if( field.section( ':', 0, 0 ) == "overlay1" )
                myDisplay.overlay1 = field.section( ':', 1 );
            else if( field.section( ':', 0, 0 ) == "center0" )
                myDisplay.center0 = field.section( ':', 1 );
            else if( field.section( ':', 0, 0 ) == "center1" )
                myDisplay.center1 = field.section( ':', 1 );
        }
    }
    emit SlimDisplayUpdate();
}

void SlimDevice::ProcessPlayingMsg( QByteArray Response )
{
    response = Response;
    // NOTE: this assumes that the MAC address of the player has been stripped off
    response.replace( "\n", " " ); // make sure there are no line feeds!!
    response = response.trimmed();
    /*
  if( response.left( 8 ) == "duration" )  // we're getting length of song
  {
    fillTimeInfo( QString( response.right( response.length() - 8 ) ).trimmed() );
    return;
  }
*/
    if( response.left( 16 ) == "playlist newsong"  ) { // it's a subscribed message regarding a new song playing on the playlist, so process it
        DEBUGF("New Song" );
        QList< QByteArray >fields = response.split( ' ' );
        devicePlaylistIndex = fields.at( 3 ).toInt();   // set index of current song
        if( getDevicePlayList().count() > 0 ) {
            deviceCurrentSongDuration = getDevicePlayList().at( devicePlaylistIndex ).duration;
        }
        if( ( devicePlaylistIndex ) >= devicePlaylistCount ) { // increment playlist index, but if it's greater than the number of songs in the playlist, update the playlist info
            DEBUGF( "Warning: playlist index is greater than the playlist size\nIndex: " << devicePlaylistIndex << "\tCount: " << devicePlaylistCount );
            //      SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,c \n" ) );
            SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,c \n" ) );
        }
        else {
            emit NewSong();
            emit playlistCoverFlowUpdate( devicePlaylistIndex );
        }
    }
    /*
  else if( response.left( 13 ) == "playlist jump"  ) { // it's a subscribed message regarding a new song playing on the playlist, so process it
    DEBUGF( "jump" );
    devicePlaylistIndex = response.right( response.lastIndexOf( ' ' ) ).toInt();
    if( devicePlaylistIndex >= devicePlaylistCount ) {
      DEBUGF( "Warning: playlist index is greater than the playlist size\nIndex: " << devicePlaylistIndex << "\tCount: " << devicePlaylistCount );
      SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,y,d,J \n" ) );
    }
    else {
      slotUpdateplaylistCoverFlow();  // update playlistCoverFlow to new song index
      emit NewSong();
      }
  }
  */
    else if( response.left( 19 ) == "playlist loadtracks" ) { // it's a subscribed message regarding a new playlist, so process it
        //    SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,y,d,J \n" ) );
        SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,c \n" ) );
        emit NewPlaylist();
    }
    else if( response.left( 18 ) == "playlist addtracks" ) { // it's a subscribed message regarding an updated playlist, so process it
        //    SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,y,d,J \n" ) );
        SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,c \n" ) );
        emit NewPlaylist();
    }
    else if( response.left( 15 ) == "playlist delete" ) { // it's a subscribed message regarding an updated playlist, so process it
//        SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,y,d,J \n" ) );
        SendDeviceCommand( QString( "status 0 1000 tags:g,a,l,t,e,y,d,c \n" ) );
        emit NewPlaylist();
    }
    else if( response.left( 12 ) == "mixer muting" ) {
        if( response.endsWith( "1" ) ) // mute
            deviceMute = true;
        else
            deviceMute = false;
        emit Mute( deviceMute );
    }
    else if( response.left( 12 ) == "mixer volume" ) {
        bool ok;
        int vol = response.right( response.lastIndexOf( ' ' ) ).toInt( &ok );
        if( ok ) {
            if( vol == 1 || vol == -1 )
                deviceVol = QByteArray::number( deviceVol.toInt() + vol );
            else
                deviceVol = QByteArray::number( vol );
            emit VolumeChange( deviceVol.toInt() );
        }
    }
    else if( response.left( 5 ) == "pause" ) {
        if( response.endsWith( "1" ) ) {
            isPlaying = false;
            emit ModeChange( "PAUSE" );
        }
        else {
            isPlaying = true;
            emit ModeChange( "PLAY" );
        }
    }
    else if( response.left( 9 ) == "mode play" ) { // current playing mode of "play", "pause" "stop"
        isPlaying = true;
        emit ModeChange( "PLAY" );
    }
    else if( response.left( 10 ) == "mode pause" ) { // current playing mode of "play", "pause" "stop"
        isPlaying = false;
        emit ModeChange( "PAUSE" );
    }
    else if( response.left( 9 ) == "mode stop" ) { // current playing mode of "play", "pause" "stop"
        isPlaying = false;
        emit ModeChange( "STOP" );
    }
    else if( response.left( 6 ) == "status" ) { // this is a status message, probably because of a new playlist or song
        ProcessStatusSetupMessage( response );
    }
    //    else if( response.left( 6 ) == "client" ) { // this is a client message -- options are connect, disconnect, reconnect
    //        emit ClientChange( deviceMAC, response.right( response.length() - 6 ).trimmed() );
    //    }
}

QByteArray SlimDevice::getDeviceRepeatModeText( void )
{
    if( deviceRepeatMode == "1" )
        return QByteArray( "SONG" );
    else if ( deviceRepeatMode == "2" )
        return QByteArray( "PLAYLIST" );
    else
        return QByteArray( "OFF" );
}

QByteArray SlimDevice::getDeviceShuffleModeText( void )
{
    if( deviceShuffleMode == "1" )
        return QByteArray( "BY SONG" );
    else if ( deviceShuffleMode == "2" )
        return QByteArray( "BY ALBUM" );
    else
        return QByteArray( "OFF" );
}

void SlimDevice::slotSetVolume( int newvol )
{
    setDeviceVol( QByteArray::number( newvol ) );
    SendDeviceCommand( QString( "mixer volume " + deviceVol + "\n" ) );
}

void SlimDevice::slotSetMute( bool mute )
{ // fixed to take care of the fact that mixer "unmuting" doesn't seem to work, so we simply reset the volume
    //  SendDeviceCommand( QString( "mixer muting " ) + ( mute ? QString( "1" ) : QString( "0" ) ) + "\n"  );
    if( mute ) {
        SendDeviceCommand( QString( "mixer muting 1 \n"  ) );   // NOTE: once in a while, this doesn't seem to work
    }
    else {
        SendDeviceCommand( QString( "mixer volume " + deviceVol + "\n" ) );
    }
}

void SlimDevice::slotSendButtonCommand( QString button )
{
    // sending a "button" command, which are listed in the Default.map file
    QString cmd = "button " + button;
    SendDeviceCommand( cmd );
}

void SlimDevice::slotSendIRCommand( int code )
{
    // note command comes in as hexidecimal, must be converted to a text representation before sending
    QString irCommand;
    int elapsedTime = playerTime.elapsed();
    irCommand = QString( "ir %1 %2 \n" ).arg( code, 0, 16 ).arg( elapsedTime, 0, 10 );
    DEBUGF( "SENDING IR COMMAND: " << irCommand );
    SendDeviceCommand( irCommand );
}

void SlimDevice::slotTick( void )
{
    if( isPlaying && songPlayingTime < songDuration ) {
        QTime t;
        int min = 0;
        int sec = 0;
        songPlayingTime++;
        min = songPlayingTime / 60;
        sec = songPlayingTime - ( 60 * min );
        t.setHMS( 0, min, sec );

        QTime s;
        min = ( int )( songDuration / 60 );
        sec = ( int )( songDuration - ( 60 * min ) );
        s.setHMS( 0, min, sec );

        timeText = QString( t.toString( QString( "mm:ss" ) ) + " of " + s.toString( QString( "mm:ss" ) ) );
        //emit TimeText( timeText );
    }

}
/* vim: set expandtab tabstop=4 shiftwidth=4: */
