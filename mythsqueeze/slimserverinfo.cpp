#include <QDir>
#include <QFile>

#include "slimserverinfo.h"
#include "slimcli.h"
#include "slimdevice.h"
#include "slimdatabasefetch.h"

#ifdef SLIMSERVERINFO_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SlimServerInfo::SlimServerInfo(QObject *parent) :
    QObject(parent)
{
    setObjectName("SlimServerInfo");
    DEBUGF("");
    //    httpPort = 9000;
    freshnessDate = 0;
}

SlimServerInfo::~SlimServerInfo()
{
    DEBUGF("");
    WriteDataFile();
}

bool SlimServerInfo::Init(SlimCLI *cliRef)
{
    DEBUGF("");
    // get cli and references sorted out
    cli = cliRef;
    SlimServerAddr = cli->GetSlimServerAddress();
    cliPort = cli->GetCliPort();

    cli->EmitCliInfo(QString("Getting Database info"));
    QByteArray cmd = QByteArray("serverstatus\n" );
    if(!cli->SendBlockingCommand(cmd))
        return false;
    if( !ProcessServerInfo(cli->GetResponse()) )
        return false;

    if( !SetupDevices()) {
        DEBUGF("NO DEVICE FOUND FROM THIS MAC");
        return false;
    }

    DEBUGF("READ DATAFILE");
    if( !ReadDataFile() )
        refreshImageFromServer();
    else
        checkRefreshDate();     // see if we need to update the database
    DEBUGF("RETURNING");
    return true;
}

bool SlimServerInfo::SetupDevices( void )
{
    DEBUGF("");
    QByteArray response;
    QList<QByteArray> respList;
    QByteArray cmd;

    cli->EmitCliInfo( "Analyzing Attached Players" );

    //    int loopCounter = 0;
    //    while(loopCounter++ < 5 ) {

    if( playerCount <= 0 ) { // we have a problem
        DEBUGF( "NO DEVICES" );
        return false;
    }
    DEBUGF("player count is:" << playerCount);

    for( int i = 0; i < playerCount; i++ ) {
        cmd = QString( "player id %1 ?\n" ).arg( i ).toAscii();
        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();
        DEBUGF(response);

        respList.clear();
        respList = response.split( ' ' );

        QString thisId;
        if( respList.at( 3 ).contains( ':' ) )
            thisId = respList.at( 3 ).toLower().toPercentEncoding(); // escape encode the MAC address if necessary
        else
            thisId = respList.at( 3 ).toLower();

        cmd = QString( "player name %1 ?\n" ).arg( i ).toAscii();

        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();
        respList.clear();
        respList = response.split( ' ' );
        DEBUGF(response);
        QString thisName = respList.at( 3 );
        //            GetDeviceNameList().insert( thisName, thisId );  // insert hash of key=Devicename value=MAC address

        cmd = QString( "player ip %1 ?\n" ).arg( i ).toAscii();

        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();
        DEBUGF(response);
        respList.clear();
        respList = response.split( ' ' );
        QString deviceIP = respList.at( 3 );
        deviceIP = deviceIP.section( QString( "%3A" ), 0, 0 );
        DEBUGF( thisId.toAscii() << " | " << thisName.toAscii()  << " | " << deviceIP.toAscii()  << " | " << i );
        deviceList.insert( thisId, new SlimDevice( thisId.toAscii(), thisName.toAscii(), deviceIP.toAscii(), QByteArray::number( i ) ) );
    }
    if(!deviceList.contains( cli->GetMACAddress().toLower()))
        return false;
    else
        return true;
}

void SlimServerInfo::InitDevices( void )
{
    DEBUGF("");
    cli->EmitCliInfo( QString( "Initializing attached devices" ) );

    QHashIterator< QString, SlimDevice* > i( deviceList );
    while (i.hasNext()) {
        i.next();
        i.value()->Init( cli );
        DEBUGF( "Initializing device: " << i.value()->getDeviceMAC() );
    }
    QByteArray command = "subscribe playlist,mixer,pause,sync,client \n";  // subscribe to playlist messages so that we can get info on it.

    cli->SendCommand( command ); // this will cause us to get messages on playlist change
    // displaystatus subscribe:all is command to get messages
    // we need to issue the command for each attached device, and since we're already at the end of the list, let's just go backwards

    while (i.hasPrevious()) {
        i.previous();
        command = i.value()->getDeviceMAC() + " displaystatus subscribe:all";
        DEBUGF( "SENDING DISPLAY COMMAND FOR DEVICE " << command );
        cli->SendCommand( command );
    }
    emit FinishedInitializingDevices();
}

SlimDevice *SlimServerInfo::GetDeviceFromMac( QByteArray mac )   // use percent encoded MAC address
{
    DEBUGF("");
    return deviceList.value( QString( mac ), NULL );
}

bool SlimServerInfo::ProcessServerInfo(QByteArray response)
{
    DEBUGF("");
    QList<QByteArray> aList = response.split( ' ' );  // split response into various pairs

    QListIterator<QByteArray> fields(aList);
    while(fields.hasNext()){
        QString line = QString(fields.next());
        if(line.section("%3A",0,0)=="lastscan")
            lastServerRefresh = line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="version")
            serverVersion=line.section("%3A",1,1).toAscii();
        else if(line.section("%3A",0,0)=="info%20total%20albums")
            totalAlbums=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="info%20total%20artists")
            totalArtists=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="info%20total%20genres")
            totalGenres=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="info%20total%20songs")
            totalSongs=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="player%20count")
            playerCount=line.section("%3A",1,1).toAscii().toInt();
    }
    return true;

}

QList<Album> SlimServerInfo::GetArtistAlbumList(QString artist)
{
    QList<Album> albumListTemp;
    QStringListIterator strIt(m_Artist2AlbumIds.value(artist));
    while(strIt.hasNext()){
        albumListTemp.append(m_AlbumID2AlbumInfo.value(strIt.next()));
    }
    return albumListTemp;
}

bool SlimServerInfo::ReadDataFile( void )
{
    DEBUGF("");
    QString dataPath = QDir::homePath()+DATAPATH;
    QDir d(dataPath);
    if(!d.exists())
        d.mkpath(dataPath);
    QDir::setCurrent(dataPath);

    QFile file;
    if( file.exists(DATAFILE) ) // there is a file, so read from it
        file.setFileName(DATAFILE);
    else {
        DEBUGF("no file to read at " << dataPath+DATAFILE);
        return false;
    }

    quint16 albumCount;

    //update the images
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);   // read the data serialized from the file

    // check version of data
    qint16 versionNo;
    in >> versionNo;
    qDebug() << versionNo << DATAVERSION;
    if(versionNo != (qint16)DATAVERSION)
        return false;


    in >> freshnessDate;
    in >> m_Artist2AlbumIds;
    in >> m_AlbumArtist2AlbumID;
    in >> albumCount;
    in >> m_albumList;
    in >> m_AlbumID2AlbumInfo;
    in >> m_artistList;

    DEBUGF("reading in info on " << albumCount << " files");

#ifdef SLIMSERVERINFO_DEBUG
    QListIterator<Artist> art(m_artistList);
    while(art.hasNext()) {
        Artist a = art.next();
        qDebug() << a.id << a.name << a.textKey;
    }

    QListIterator<Album> al(m_albumList);
    while(al.hasNext()) {
        Album a = al.next();
        qDebug() << a.albumTextKey << a.albumtitle << a.artist;
    }
#endif

    DEBUGF( "Reading file of size: " << file.size() );
    file.close();
    return true;
}

void SlimServerInfo::WriteDataFile( void )
{
    DEBUGF("");
    QString dataPath = QDir::homePath()+DATAPATH;
    QDir d(dataPath);
    if(!d.exists())
        d.mkpath(dataPath);
    QDir::setCurrent(dataPath);

    QFile file;
    file.setFileName(DATAFILE);

    //update the information
    if(!file.open(QIODevice::WriteOnly)) {
        DEBUGF("Error opening file for writing");
        return;
    }
    QDataStream out(&file);   // read the data serialized from the file
    out << (qint16)DATAVERSION; // set data verion number

    out << lastServerRefresh;
    out << m_Artist2AlbumIds;
    out << m_AlbumArtist2AlbumID;
    out << (qint16)m_AlbumID2AlbumInfo.count();
    out << m_albumList;
    out << m_AlbumID2AlbumInfo;
    out << m_artistList;

    DEBUGF( "Writing file of size: " << file.size() );
    file.close();
    return;
}

void SlimServerInfo::checkRefreshDate(void)
{
    DEBUGF("");
    if(lastServerRefresh!=freshnessDate)
        refreshImageFromServer();
}

void SlimServerInfo::refreshImageFromServer(void)
{
    DEBUGF("");
    db = new SlimDatabaseFetch();
    connect(db,SIGNAL(FinishedUpdatingDatabase()),
            this,SLOT(DatabaseUpdated()));

    db->Init(SlimServerAddr,cliPort,cli->GetCliUsername(),cli->GetCliPassword());
    db->start();    // init database fetching thread
}

void SlimServerInfo::DatabaseUpdated(void)
{
    DEBUGF("");
    m_AlbumArtist2AlbumID = db->AlbumArtist2AlbumID();
    m_AlbumID2AlbumInfo = db->AlbumID2AlbumInfo();
    m_Artist2AlbumIds = db->Artist2AlbumIds();
    m_albumList = db->GetAllAlbumList();
    m_artistList = db->GetAllArtistList();
    db->exit();
    db->deleteLater();
}
