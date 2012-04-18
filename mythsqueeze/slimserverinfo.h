#ifndef SLIMSERVERINFO_H
#define SLIMSERVERINFO_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QPixmap>
#include <QString>
#include <QByteArray>


#include "squeezedefines.h"


class SlimServerInfo : public QObject
{
    Q_OBJECT
public:
    explicit SlimServerInfo(QObject *parent = 0);
    ~SlimServerInfo();
    bool Init(SlimCLI *cliRef);

    bool SetupDevices( void );
    void InitDevices( void );
    bool GetDatabaseInfo(void);

    SlimDevices GetDeviceList( void ) { return deviceList; }
    SlimDevice *GetDeviceFromMac( QByteArray mac );   // use percent encoded MAC address, return pointer to associated device
    SlimDevice *GetCurrentDevice( void ) { return currentDevice; }
    void SetCurrentDevice( SlimDevice *d ) { currentDevice = d; }

//    QPixmap GetAlbumArt( QString album, QString artist );
//    QPixmap GetAlbumArt(QString coverid) { return m_Id2Art.value(coverid,NULL); }
    QList<Album> GetArtistAlbumList(QString artist);

    SlimItem AlbumArtist2AlbumID(void) {return m_AlbumArtist2AlbumID;}
    SlimItemList Artist2AlbumIds(void) {return m_Artist2AlbumIds;}
    SlimAlbumItem AlbumID2AlbumInfo(void) {return m_AlbumID2AlbumInfo;}    // AlbumID to Album Info
    QList<Album> GetAllAlbumList(void) {return m_albumList;}
    QList<Artist> GetAllArtistList(void) {return m_artistList;}

    bool ReadDataFile( void );   // read in existing images (if any) into serverImageList (save bandwidth)
    void WriteDataFile( void );  // write out contents of serverImageList

signals:
   void FinishedInitializingDevices(void);

public slots:
   void DatabaseUpdated(void);

private:
    bool ProcessServerInfo(QByteArray resp);
    void checkRefreshDate(void);
    void refreshImageFromServer(void);

    int totalAlbums;
    int totalArtists;
    int totalGenres;
    int totalSongs;
    int playerCount;
    QByteArray serverVersion;
    qint16 lastServerRefresh;
    qint16 freshnessDate;

    SlimDevice *currentDevice;  // convenience pointer
//    SlimItem deviceMACList;     // MAC address associated with SlimDevice
    SlimDevices deviceList;     // convert MAC address to SlimDevice pointer

    SlimCLI *cli;               // convenience pointer
    QString SlimServerAddr;   // server IP address
    quint16 cliPort;          // port to use for cli, usually 9090, but we allow the user to change this
//    quint16 httpPort;         // port for http connection to retrieve images, usually 9000.

    SlimItem m_AlbumArtist2AlbumID;         // Album+Artist name to Album ID
    SlimItemList m_Artist2AlbumIds;         // Artist name to list of albums
    SlimAlbumItem m_AlbumID2AlbumInfo;      // AlbumID to Album Info
    QList<Album> m_albumList;
    QList<Artist> m_artistList;             // alpha sorted list of artist names

    SlimDatabaseFetch *db;      // pointer in case we need to update the database
    bool bNeedRefresh;          // the SqueezeServer database was updated since the last time we connected, so refresh
};


#endif // SLIMSERVERINFO_H
