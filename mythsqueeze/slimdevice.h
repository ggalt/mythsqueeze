#ifndef SLIMDEVICE_H
#define SLIMDEVICE_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QPixmap>
#include <QRect>
#include <QEvent>
#include <QWidget>


#include "squeezedefines.h"

class SlimDevice : public QObject
{
    Q_OBJECT
public:
    SlimDevice( QByteArray MAC, QByteArray Name, QByteArray IP, QByteArray number, QObject *parent=0, const char *name = NULL );
    SlimDevice( QObject *parent=0, const char *name = NULL );
    ~SlimDevice( void );

    void Init( SlimCLI *cli );
    void ProcessStatusSetupMessage( QByteArray msg );

    void SendDeviceCommand( QString cmd );
    void SetplaylistCoverFlowRect( QRect r );
    QByteArray  getDeviceNumber( void ) { return deviceNumber; }
    void        setDeviceNumber( QByteArray devicenumber ) { deviceNumber = devicenumber; }
    // -----------------------------------------------
    QByteArray  getDeviceMAC( void ) { return deviceMAC; }
    void        setDeviceMAC( QByteArray devicemac ) { deviceMAC = devicemac; }
    // -----------------------------------------------
    QByteArray  getDeviceName( void ) { return deviceName; }
    void        setDeviceName( QByteArray devicename ) { deviceName = devicename; }
    // -----------------------------------------------
    QByteArray  getDeviceIP( void ) { return deviceIP; }
    void        setDeviceIP( QByteArray deviceip ) { deviceIP = deviceip; }
    // -----------------------------------------------
    bool        getDeviceConnected( void ) { return deviceConnected; }
    void        setDeviceConnected( bool deviceconnected ) { deviceConnected = deviceconnected; }
    // -----------------------------------------------
    QByteArray  getDeviceSleep( void ) { return deviceSleep; }
    void        setDeviceSleep( QByteArray devicesleep ) { deviceSleep = devicesleep; }
    // -----------------------------------------------
    QList<QByteArray> getDeviceSync( void ) { return deviceSync; }
    void        setDeviceSync( QList<QByteArray> devicesync ) { deviceSync = devicesync; }
    // -----------------------------------------------
    bool        getDevicePower( void ) { return devicePower; }
    void        setDevicePower( bool devicepower ) { devicePower = devicepower; }
    // -----------------------------------------------
    QByteArray  getDeviceVol( void ) { return deviceVol; }
    void        setDeviceVol( QByteArray devicevol ) { deviceVol = devicevol; }
    // -----------------------------------------------
    bool        getDeviceMute( void ) { return deviceMute; }
    void        setDeviceMute( bool devicemute ) { deviceMute = devicemute; }
    // -----------------------------------------------
    QByteArray  getDeviceRate( void ) { return deviceRate; }
    void        setDeviceRate( QByteArray devicerate ) { deviceRate = devicerate; }
    // -----------------------------------------------
    QByteArray  getDeviceMode( void ) { return deviceMode; }
    void        setDeviceMode( QByteArray devicemode ) { deviceMode = devicemode; }
    // -----------------------------------------------
    QByteArray  getDeviceCurrentSongTime( void ) { return deviceCurrentSongTime; }
    void        setDeviceCurrentSongTime( QByteArray devicecurrentsongtime ) { deviceCurrentSongTime = devicecurrentsongtime; }
    // -----------------------------------------------
    QByteArray  getDeviceCurrentSongDuration( void ) { return deviceCurrentSongDuration; }
    void        setDeviceCurrentSongDuration( QByteArray devicecurrentsongduration ) { deviceCurrentSongDuration = devicecurrentsongduration; }
    // -----------------------------------------------
    QByteArray  getDevicePlaylistName( void ) { return devicePlaylistName; }
    void        setDevicePlaylistName( QByteArray deviceplaylistname ) { devicePlaylistName = deviceplaylistname; }
    // -----------------------------------------------
    int         getDevicePlaylistCount( void ) { return devicePlaylistCount; }
    void        setDevicePlaylistCount( int deviceplaylistcount ) { devicePlaylistCount = deviceplaylistcount; }
    // -----------------------------------------------
    int         getDevicePlaylistIndex( void ) { return devicePlaylistIndex; }
    void        setDevicePlaylistIndex( int deviceplaylistindex ) { devicePlaylistIndex = deviceplaylistindex; }
    // -----------------------------------------------
    QByteArray getDeviceRepeatMode( void ) { return deviceRepeatMode; }
    QByteArray getDeviceShuffleMode( void ) { return deviceShuffleMode; }
    QByteArray getDeviceRepeatModeText( void );
    QByteArray getDeviceShuffleModeText( void );
    // -----------------------------------------------
    QList<QByteArray> getDevicePlaylistTitles( void ) { return devicePlaylistTitles; }
    void        setDevicePlaylistTitles( QList<QByteArray> deviceplaylisttitles ) { devicePlaylistTitles = deviceplaylisttitles; }
    // -----------------------------------------------
    QList<QByteArray> getDevicePlaylistArtists( void ) { return devicePlaylistArtists; }
    void        setDevicePlaylistArtists( QList<QByteArray> deviceplaylistartists ) { devicePlaylistArtists = deviceplaylistartists; }
    // -----------------------------------------------
    QList<QByteArray> getDevicePlaylistAlbums( void ) { return devicePlaylistAlbums; }
    void        setDevicePlaylistAlbums( QList<QByteArray> deviceplaylistalbums ) { devicePlaylistAlbums = deviceplaylistalbums; }
    // -----------------------------------------------
    QList<QByteArray> getDevicePlaylistGenres( void ) { return devicePlaylistGenres; }
    void        setDevicePlaylistGenres( QList<QByteArray> deviceplaylistgenres ) { devicePlaylistGenres = deviceplaylistgenres; }
    // -----------------------------------------------
    QList<QByteArray> getDevicePlaylistYears( void ) { return devicePlaylistYears; }
    void        setDevicePlaylistYears( QList<QByteArray> deviceplaylistyears ) { devicePlaylistYears = deviceplaylistyears; }
    // -----------------------------------------------
    CurrentPlayList getDevicePlayList( void ) { return devicePlayList; }
    void        setDevicePlaylist( CurrentPlayList DevicePlayList ) { devicePlayList = DevicePlayList; }

    TrackData getCurrentTrack( void ) { return currentTrack; }
    DisplayBuffer *getDisplayBuffer( void ) { return &myDisplay; }

    SlimCLI *GetCLIInterface( void ) { return cliInterface; }

public slots:
    void ProcessDisplayStatusMsg( QByteArray Response );
    void ProcessPlayingMsg( QByteArray Response );
    void ResponseReceiver( QByteArray Response ) { response = Response; }
    void slotSetVolume( int newvol );
    void slotPlay( bool play )
    { ( play ?
        this->SendDeviceCommand( QString( "pause 0\n" ) )
    :
    this->SendDeviceCommand( QString( "pause 1\n" ) ) );
    }
    void slotRewind( void ) { this->SendDeviceCommand( QString( "button rew.single\n" ) ); }
    void slotFastForward( void ) { this->SendDeviceCommand( QString( "button fwd.single\n" ) ); }
    void slotSetMute( bool mute );
    void slotSendButtonCommand( QString button );
    void slotSendIRCommand( int code );

    void slotRepeatMode( QByteArray mode ) { this->SendDeviceCommand( QString( "playlist repeat " + mode + " \n" ) ); }
    void slotShuffleMode( QByteArray mode ) { this->SendDeviceCommand( QString( "playlist shuffle " + mode + " \n" ) ); }

    void slotTick( void );

signals:
    void NewSong( void );
    void NewPlaylist( void );
    void VolumeChange( int newvol );
    void ModeChange( QString newmode );
    void Mute( bool mute );
    void SlimDisplayUpdate( void );
    void playlistCoverFlowUpdate( int );
    void playlistCoverFlowCreate( void );
    void DeviceReady(void);

private:

    SlimCLI *cliInterface;  // hold a pointer to the cli interface so we can work with it.
    QByteArray response;    // hold the responses we get

    /*
* NOTE: almost all of the variables are exchanged with the server as
* string-based commands and so we store virutually everything as
* QByteArrays rather than as the actual item represented (e.g., an 'int')
* so that server communication doesn't require a bunch of conversions back
* and forth -- except in rare instances.
*/
    QByteArray deviceNumber; // device number in "Device count" (i.e., if there are 3 devices, which number (0,1,2) is this?)
    QByteArray deviceMAC; // MAC address of the device
    QByteArray deviceName; // Name as listed in server (e.g., "SoftSqueeze" or "Kitchen Device")
    QByteArray deviceIP; // IP address of device

    bool deviceConnected; // is device connected (always true if device is a Slimp3)
    QByteArray deviceSleep; // number of seconds left before device goes to sleep
    QList<QByteArray> deviceSync; // device(s) this device is sync'd with
    bool devicePower; // is device on or off?
    QByteArray deviceVol; // volume (0-100)
    bool deviceMute; // is device muted
    QByteArray deviceRate; // rate of play (e.g., normal, 2X, etc)
    QByteArray deviceMode; // one of the following: "play", "stop" or "pause"
    QByteArray deviceRepeatMode;
    QByteArray deviceShuffleMode;
    QByteArray deviceCurrentSongTime; // time into current song
    QByteArray deviceCurrentSongDuration; // length of current song
    bool isPlaying;

    QByteArray devicePlaylistName; // name of current playlist
    int devicePlaylistCount; // number of tracks in current playlist
    int devicePlaylistIndex;  // where are we in the current playlist
    QList<QByteArray> devicePlaylistTitles; // titles of songs in playlist
    QList<QByteArray> devicePlaylistArtists; // artists associated with titles in playlist
    QList<QByteArray> devicePlaylistAlbums; // album associated with titles in playlist
    QList<QByteArray> devicePlaylistGenres; // genres associated with titles in playlist
    QList<QByteArray> devicePlaylistYears; // years associated with titles in playlist
    CurrentPlayList devicePlayList; // all info related to the current device playlist
    bool doneGettingDeviceSettings;
    bool initializingDeviceSettings;  // marks whether or not we're getting the device settings for the first time
    float songDuration;
    int songPlayingTime;
    bool startingMidSong;
    TrackData currentTrack;
    QTime songTime;
    QString timeText;
    DisplayBuffer myDisplay;
    bool coversNotReady;

    QTime playerTime;   // how long have we been playing?
};


#endif // SLIMDEVICE_H
