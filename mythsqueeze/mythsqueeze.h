#ifndef MYTHSQUEEZE_H
#define MYTHSQUEEZE_H

// MythTV headers
#include <mythtv/verbosedefs.h>
#include <mythtv/mythcontext.h>
#include <mythtv/libmythui/mythmainwindow.h>
#include <mythtv/mythdirs.h>
#include <mythtv/libmythui/mythscreentype.h>
#include <mythtv/libmythui/mythuitext.h>
#include <mythtv/libmythui/mythuivideo.h>
#include <mythtv/libmythui/mythuiimage.h>
#include <mythtv/libmythui/mythfontproperties.h>
#include <mythtv/libmythui/mythpainter.h>

// Qt declarations
#include <QtGui/QMainWindow>
#include <QtWebKit/QtWebKit>
#include <QObject>
#include <QtGlobal>
#include <QApplication>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QBrush>
#include <QPalette>
#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QPoint>
#include <QProcess>
#include <QPixmap>
#include <QKeyEvent>
#include <QTime>
#include <QTimer>
#include <QNetworkInterface>
#include <QMainWindow>
#include <QLabel>
#include <QUrl>
#include <QSplashScreen>
#include <QSettings>
#include <QColorDialog>
#include <QIcon>

// Local App declarations
#include "squeezedefines.h"
#include "slimcli.h"
#include "slimdevice.h"
#include "mythsqueezedisplay.h"


/** \class MythSqueeze
 *
 */
class MythSqueeze : public MythScreenType
{
    Q_OBJECT

public:

    MythSqueeze(MythScreenStack *parent, const char *name);
    ~MythSqueeze();

    bool Create(void);
    void InitPlayer( void );
    bool keyPressEvent(QKeyEvent *e);
    void customEvent(QEvent*);

public slots:
    void SqueezePlayerError( void );
    void SqueezePlayerOutput( void );

    void slotUpdateplaylistCoverFlow( int trackIndex );
    void slotCreateplaylistCoverFlow( void );
    void slotplaylistCoverFlowReady(void);

    void SetupSelectionCoverFlows(void);
    void ArtistAlbumCoverFlowSelect(void);
    void AlbumCoverFlowSelect(void);
    void ResetKeypadTimer(void);
    void ChangeCoverflowDisplay(void);
//    void ChangeToAlbumSelection(int tab);

    void slotDisablePlayer( void );
    void slotEnablePlayer( void );
    void slotSetActivePlayer( void );
    void slotSetActivePlayer( SlimDevice *d );

    void saveDisplayConfig(void);
    void saveConnectionConfig(void);
    void loadConfig(void);
    void setConfigDisplay2Defaults(void);
    void setConfigConnection2Defaults(void);

    void setCoverFlowColor(void);
    void setDisplayBackgroundColor(void);
    void setDisplayTextColor(void);

    // button commands from Default.map file
    void slotRewind( void ) { activeDevice->SendDeviceCommand( QString( "button rew.single\n" ) ); }
    void slotPrev( void ) { activeDevice->SendDeviceCommand( QString( "button rew\n" ) ); }
    void slotStop( void ) { activeDevice->SendDeviceCommand( QString( "button pause.hold\n" ) ); }
    void slotPlay( void );
    void slotPause( void ) { activeDevice->SendDeviceCommand( QString( "button pause\n" ) ); }
    void slotFForward( void ) { activeDevice->SendDeviceCommand( QString( "button fwd.single\n" ) ); }
    void slotAdd( void );
    //  void slotPower( void ) { activeDevice->SendDeviceCommand( QString( "button power\n" ) ); close(); }
    void slotPower( void ) { close(); }
    void slotVolUp( void ) { activeDevice->SendDeviceCommand( QString( "button volup\n" ) ); }
    void slotVolDown( void ) { activeDevice->SendDeviceCommand( QString( "button voldown\n" ) ); }
    void slotMute( void ) { activeDevice->SendDeviceCommand( QString( "button muting\n" ) ); }
    void slotShuffle( void ) { activeDevice->SendDeviceCommand( QString( "button shuffle\n" ) ); }
    void slotRepeat( void ) { activeDevice->SendDeviceCommand( QString( "button repeat\n" ) ); }
    void slotBright( void ) { activeDevice->SendDeviceCommand( QString( "button brightness\n" ) ); }
    void slotSize( void ) { activeDevice->SendDeviceCommand( QString( "button size\n" ) ); }
    void slotSearch( void ) { activeDevice->SendDeviceCommand( QString( "button search\n" ) ); }
    void slotSleep( void ) { activeDevice->SendDeviceCommand( QString( "button sleep\n" ) ); }
    void slotLeftArrow( void );
    void slotRightArrow( void );
    void slotUpArrow( void );
    void slotDownArrow( void );
    void slotNowPlaying( void ) { activeDevice->SendDeviceCommand( QString( "button now_playing\n" ) ); }
    void slot0PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 0\n" ) ); }
    void slot1PAD( void ) { activeDevice->SendDeviceCommand( QString( "button 1\n" ) ); }
    // BUTTONS 2-9 are handled differently because we want them
    // to update the "artist" and "album" coverflows as well
    void slot2PAD( void ){UpdateCoverflowFromKeypad(2);}
    void slot3PAD( void ){UpdateCoverflowFromKeypad(3);}
    void slot4PAD( void ){UpdateCoverflowFromKeypad(4);}
    void slot5PAD( void ){UpdateCoverflowFromKeypad(5);}
    void slot6PAD( void ){UpdateCoverflowFromKeypad(6);}
    void slot7PAD( void ){UpdateCoverflowFromKeypad(7);}
    void slot8PAD( void ){UpdateCoverflowFromKeypad(8);}
    void slot9PAD( void ){UpdateCoverflowFromKeypad(9);}

protected:
    void resizeEvent(QResizeEvent *e);

private:
    void getplayerMACAddress( void );

    QProcess *squeezePlayer;
    SlimCLI *slimCLI;
    SlimDevice *activeDevice;
    QTime progstart;

    // for display of the slim device interface
    MythSqueezeDisplay *m_disp;

    // mythtv interface elements
    MythUIText        *m_playerName;
//    MythUIImage       *m_slimDisplay;
    MythScreenStack    *m_popupStack;
    MythUIVideo *m_squeezeDisplay;


    QList<QByteArray> outDevs;
    QColor displayBackground;
    QColor displayTextColor;
    QColor tempdisplayTextColor;
    QColor tempdisplayBackground;
    quint16 scrollInterval;
    quint16 scrollSpeed;
    QString lmsUsername;
    QString lmsPassword;

    QByteArray MacAddress;      // MAC address of this machine (which will become the MAC address for our player)
    QString SlimServerAddr;   // server IP address
    QString SlimServerAudioPort;  // address for audio connection, default 3483
    QString SlimServerCLIPort;    // address for CLI interfact, default 9090
    QString SlimServerHttpPort;       // address for http connection, default 9000
    QString PortAudioDevice;    // device to use for PortAudio -- leave blank for default device

    QTimer keypadTimer;
    int lastKey;
    int keyOffset;
    QString lastMenuHeading0;
    QString lastMenuHeading1;

    bool isStartUp;
};

#endif // MYTHSQUEEZE_H
