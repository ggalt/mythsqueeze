#include <QHostAddress>

#include "slimdatabasefetch.h"
#include "slimserverinfo.h"

#ifdef SLIMDATABASE_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SlimDatabaseFetch::SlimDatabaseFetch(QObject *parent) :
    QThread(parent)
{
    DEBUGF("");
    SlimServerAddr = "127.0.0.1";
    cliPort = 9090;        // default, but user can reset
    MaxRequestSize = "500";    // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)
    gotAlbums=false;
    gotArtists=false;
}

SlimDatabaseFetch::~SlimDatabaseFetch(void)
{
    DEBUGF("database destroyed");
}

void SlimDatabaseFetch::Init(QString serveraddr, qint16 cliport,
                             QString cliuname, QString clipass)
{
    DEBUGF("");
    SlimServerAddr = serveraddr;
    cliPort = cliport;
    cliUsername = cliuname;
    cliPassword = clipass;

}

void SlimDatabaseFetch::run()
{
    DEBUGF("");
    slimCliSocket = new QTcpSocket();
    connect(slimCliSocket,SIGNAL(connected()),
            this,SLOT(cliConnected()));
    connect(slimCliSocket,SIGNAL(disconnected()),
            this,SLOT(cliDisconnected()));
    connect(slimCliSocket,SIGNAL(error(QAbstractSocket::SocketError)),
            this,SLOT(cliError(QAbstractSocket::SocketError)));
    connect(slimCliSocket,SIGNAL(readyRead()),
            this,SLOT(cliMessageReady()));

    slimCliSocket->connectToHost(QHostAddress(SlimServerAddr),cliPort);

    exec();     // start event loop
}


// --------------------------------- SLOTS --------------------------------------

void SlimDatabaseFetch::cliConnected(void)
{
    DEBUGF("");
    QByteArray cmd;
    if(!cliUsername.isNull()){ // username present, get authentication
        cmd = QString("login %1 %2\n" )
                .arg( cliUsername )
                .arg( cliPassword ).toAscii();
        slimCliSocket->write(cmd);      // NOTE: no relevant response given except for a disconnect on error
    }
    cmd = QString("albums 0 %1 tags:t,y,a,S,j,s\n").arg(QString(MaxRequestSize)).toAscii();  // = album title, year, artist, artist_id, artwork_track_id
    slimCliSocket->write(cmd);      // send the request, which will start the process
}

void SlimDatabaseFetch::cliDisconnected(void)
{
    DEBUGF("CLI disconnected");
}

void SlimDatabaseFetch::cliError(QAbstractSocket::SocketError socketError)
{
    DEBUGF("CLI Error: " << socketError);
}

void SlimDatabaseFetch::cliMessageReady(void)
{
    DEBUGF("");
    iTimeOut = 10000;
    QTime t;
    t.start();
    while( slimCliSocket->bytesAvailable() ) {
        while( !slimCliSocket->canReadLine () ) { // wait for a full line of content  NOTE: protect against unlikely infinite loop with timer
            if( t.elapsed() > iTimeOut ) {
                return;
            }
        }
        response = slimCliSocket->readLine();
        RemoveNewLineFromResponse();
        if( !ProcessResponse() )
            return;
        if( t.elapsed() > 2*iTimeOut)   // there is some issue, and we don't want to loop forever
            return;
    }
}

//------------------------------- HELPER FUNCTIONS ---------------------------------------------
void SlimDatabaseFetch::RemoveNewLineFromResponse( void )
{
    DEBUGF("");
    while( response.contains( '\n' ) )
        response.replace( response.indexOf( '\n' ), 1, " " );

}

bool SlimDatabaseFetch::ProcessResponse(void)
{
    DEBUGF("");
    if( !response.contains( ':' ) )  // Substitute ':' for '%3A', we'll URL::decode later
        response.replace( "%3A", ":" );

    QList<QByteArray> list = response.split( ' ' ); // split into a list of the fields

    // NOTE: first three fields are the original command (e.g., "albums 0 500") and response ends with "count%3A<total albums>"
    // these next items are to retain the relevant info on the original request so that we can determine what we are processing, and
    // to ask for more data if we need to (i.e., there are more than 500 items in the list)

    QString requestType;
    QString startPosition;
    QString endPosition;
    QString tagLine;
    int ReceiveCount = MaxRequestSize.toInt();  // initialize to standard request size, just in case
    int c = 0;

    bool gotAlbumID = false;    // set this so that the first time through, we don't try to write out a bunch of garbage
    bool gotArtistID = false;

    QByteArray album_id;
    QByteArray artist_name;
    QByteArray artist_id;
    QByteArray cover_id;
    QByteArray album_title;
    QByteArray album_year;
    QByteArray textkey;

    for( QList<QByteArray>::Iterator it = list.begin(); it != list.end(); ++it, c++ )
    {
        QString field = QUrl::fromPercentEncoding( *it );   // convert from escape encoded

        // we want to preserve the fields 0-2, which contain the original command (e.g., "albums 0 10 tags:t,y,a,S,j,s") and ignore
        // the fourth field, which contains the tags requested
        switch( c )
        {
        case 0:
            requestType = field.trimmed();
            qDebug() << "REquest type is: " << requestType << c << field;
            break;
        case 1:
            startPosition = field.trimmed();
            break;
        case 2:
            endPosition = field.trimmed();
            // our start position plus our requested # of items gives the total count of
            // received items so far (e.g., albums 0 100 means 100 items retrieved so far
            // (0+100 = 100), but the next request "albums 100 100" at the end means we've
            // retrieved 200 albums so far (100+100=200), with the next request of
            // "albums 200 100" retrieving the next 100 for a total of 300 (200+100))
            ReceiveCount = endPosition.toInt() + startPosition.toInt();
            break;
        case 3: // fourth field of "tags:l%2Cj%2Ca"
            tagLine = field.trimmed();
            break;
        default:	// this is where most of the processing will occur
            if(requestType.toLower()=="albums") {
                if( field.section( ':', 0, 0 ).trimmed() == "id" ) {
                    // "id" is the first field, and we need to capture it, but it also triggers gathering the information
                    // so for the very first time through, we simply skip the data collection and writing step
                    // since we haven't gathered any data yet
                    if(!gotAlbumID)
                        gotAlbumID = true;
                    else {
                        // process what we've retrieved so far under the last album_id
                        // into the album info structure
                        Album a;
                        a.albumTextKey = textkey;
                        a.album_id = album_id;
                        a.artist = artist_name;
                        a.artist_id = artist_id;
                        a.coverid = cover_id;
                        a.albumtitle = album_title;
                        a.year = album_year;
                        a.artist_album = album_title.trimmed().toUpper()+artist_name.trimmed().toUpper();
                        m_AlbumArtist2AlbumID.insert(QString(album_title.trimmed()+artist_name.trimmed()), QString(album_id.trimmed()));
                        m_AlbumID2AlbumInfo.insert(album_id.trimmed(),a);
                        m_albumList.append(a);
                        DEBUGF("Adding album" << a.artist << a.albumtitle << m_albumList.count());
                        if(m_Artist2AlbumIds.contains(artist_name.trimmed())) {
                            QStringList temp = m_Artist2AlbumIds.value(artist_name.trimmed());
                            temp.append(album_id.trimmed());
                            m_Artist2AlbumIds.insert(QString(artist_name.trimmed()),temp);     // removes existing record for artist and substitutes new list
                        }
                        else {
                            QStringList album_id_list;
                            album_id_list.append(album_id.trimmed());
                            m_Artist2AlbumIds.insert(artist_name.trimmed(),album_id_list);
                        }
                    }

                    // start over
                    // clear all of the values in case the next album doesn't contain one of them
                    album_id.clear();
                    artist_name.clear();
                    artist_id.clear();
                    cover_id.clear();
                    album_title.clear();
                    album_year.clear();
                    textkey.clear();

                    // if the first field is an "id", then we know we have an "id:<album id number>"
                    album_id = field.section( ':', 1, 1 ).toAscii();	// preserve the id tag
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "year" ) {
                    album_year = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "artwork_track_id" ) {
                    cover_id = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "title" ) {
                    album_title = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "artist_id" ) {
                    artist_id = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "artist" ) {
                    artist_name = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "textkey" ) {
                    textkey = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "count" )	// last field is the total count of items of this type, if more than 500, get the rest
                {
                    QString thisCount = field.section( ':', 1, 1 );	// OK, what is the real number of items available from the server
                    int fullCount = thisCount.toInt();  // make it a number for convenience

                    if( fullCount > ReceiveCount )	// if we didn't get enough, get the rest
                    {
                        QByteArray cmd = QString("albums %1 %2 tags:t,y,a,S,j,s\n").arg(ReceiveCount).arg(QString(MaxRequestSize)).toAscii();
                        slimCliSocket->write(cmd);
                        return true;
                    }
                    else {
                        gotAlbums = true;
                        QByteArray cmd = QString("artists 0 %1 tags:s\n").arg(QString(MaxRequestSize)).toAscii();  // = album title, year, artist, artist_id, artwork_track_id
                        slimCliSocket->write(cmd);      // send the request, which will start the process
                    }

                }	// end of else/if field.section == count
            }   // end of if(requestType=="albums")
            else if(requestType.toLower()=="artists") {
                if( field.section( ':', 0, 0 ).trimmed() == "id" ) {
                    // "id" is the first field, and we need to capture it, but it also triggers gathering the information
                    // so for the very first time through, we simply skip the data collection and writing step
                    // since we haven't gathered any data yet
                    if(!gotArtistID)
                        gotArtistID = true;
                    else {
                        // process what we've retrieved so far under the last artist_id
                        // into the artist info structure
                        Artist a;
                        a.id = artist_id;
                        a.name = artist_name;
                        a.textKey = textkey;

                        m_artistList.append(a);
                        DEBUGF("Adding artist" << a.name << a.id << m_artistList.count());
                    }

                    // start over
                    // clear all of the values in case the next album doesn't contain one of them
                    artist_name.clear();
                    artist_id.clear();
                    textkey.clear();

                    // if the first field is an "id", then we know we have an "id:<album id number>"
                    artist_id = field.section( ':', 1, 1 ).toAscii();	// preserve the id tag
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "artist" ) {
                    artist_name = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "textkey" ) {
                    textkey = field.section( ':', 1, 1 ).toAscii();
                }
                else if( field.section( ':', 0, 0 ).trimmed() == "count" )	// last field is the total count of items of this type, if more than 500, get the rest
                {
                    QString thisCount = field.section( ':', 1, 1 );	// OK, what is the real number of items available from the server
                    int fullCount = thisCount.toInt();  // make it a number for convenience

                    if( fullCount > ReceiveCount )	// if we didn't get enough, get the rest
                    {
                        QByteArray cmd = QString("artists %1 %2 tags:s\n").arg(ReceiveCount).arg(QString(MaxRequestSize)).toAscii();
                        slimCliSocket->write(cmd);
                        return true;
                    }
                    else {
                        gotArtists= true;
                        emit FinishedUpdatingDatabase();
                    }
                }
            } // end of if(requestType=="artists")
            else {
                emit FinishedUpdatingDatabase();
            }

        }	// end of switch statement
    }		// end for loop
    return true;
}


/* vim: set expandtab tabstop=4 shiftwidth=4: */

