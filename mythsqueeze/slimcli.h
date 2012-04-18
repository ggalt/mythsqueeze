#ifndef SLIMCLI_H
#define SLIMCLI_H

// QT4 Headers
#include <QObject>
#include <QTcpSocket>
#include <QBuffer>
#include <QTimer>
#include <QVector>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QByteArray>
#include <QList>
#include <QPixmap>
//#include <QFile>
//#include <QDataStream>

#include "squeezedefines.h"

/*
  * NOTE: This class establishes an object to communicate with a SqueezeCenter Server
  * via the SqueezeCenter command line interface (usually located at port 9090).  You
  * MUST set the IP address of the SqueezeCenter server and the port BEFORE you call init().
  * Otherwise it will default to the localhost and port 9090.
*/

class SlimCLI : public QObject {
  Q_OBJECT

public:
  SlimCLI( QObject *parent=0, const char *name = NULL );
  ~SlimCLI();

  void Connect( void );
  void Init( void );

  bool SendCommand( QByteArray cmd );
  bool SendBlockingCommand( QByteArray cmd );
  QByteArray GetBlockingCommandResponse( QByteArray cmd );    // send a blocking command and get the response back

  QByteArray GetResponse( void ) { return response; }
  QByteArray MacAddressOfResponse( void );
  QByteArray ResponseLessMacAddress( void );
  void RemoveNewLineFromResponse( void );

  // ------------------------------------------------------------------------------------------
//  bool    SetMACAddress( unsigned char *macaddr );      // used with either a 6 char array, or a full xx:xx:xx:xx structure
  bool    SetMACAddress( QString addr );
  QByteArray GetMACAddress( void ) { return macAddress; }

  void SetServerInfo(SlimServerInfo *s) { serverInfo = s; }
  SlimServerInfo *GetServerInfo(void) { return serverInfo; }
  // ------------------------------------------------------------------------------------------
  void    SetSlimServerAddress( QString addr );
  QString GetSlimServerAddress( void ) { return SlimServerAddr; }
  // ------------------------------------------------------------------------------------------
  void    SetCliPort( int port );
  void    SetCliPort( QString port );
  int     GetCliPort( void ) { return cliPort; }
  // ------------------------------------------------------------------------------------------
  void    SetCliUsername( QString username );
  QString GetCliUsername( void ) { return cliUsername; }
  // ------------------------------------------------------------------------------------------
  void    SetCliPassword( QString password );
  QString GetCliPassword( void ) { return cliPassword; }
  // ------------------------------------------------------------------------------------------
  void    SetCliMaxRequestSize( QByteArray max ) { MaxRequestSize = max; }
  void    SetCliTimeOutPeriod( QString t ) { iTimeOut = t.toInt(); }    // NOTE: NO ERROR CHECKING!!!
  // ------------------------------------------------------------------------------------------
  bool    NeedAuthentication( void ) { return useAuthentication; }
  // ------------------------------------------------------------------------------------------
  void EmitCliInfo(QString msg) { emit cliInfo(msg);}

  //  SlimDevices GetDeviceList( void ) { return deviceList; }
//  SlimDevice *GetDeviceFromMac( QByteArray mac );   // use percent encoded MAC address
//  SlimDevice *GetCurrentDevice( void ) { return currentDevice; }
////  SlimItem GetDeviceNameList( void ) { return deviceNameList; }
//  void SetCurrentDevice( SlimDevice *d ) { currentDevice = d; }

signals:
  void isConnected( void );               // we're connected to the SqueezeCenter server
  void cliError( int errmsg , const QString &message = "" );
  void cliInfo( QString msg );
  void FinishedInitializingDevices( void );

public slots:

private:
  QTcpSocket *slimCliSocket;// socket for CLI interface

  QByteArray command;       // string to build a command (different from "currCommand" below that is used to check what the CLI sends back
  QByteArray response;      // buffer to hold CLI response
  QList<QByteArray> responseList; // command response processed into "tag - response" pairs

//  SlimDevice *currentDevice;  // convenience pointer that we can return with a locked mutex
////  SlimItem deviceNameList;    // name to MAC address conversion
//  SlimItem deviceMACList;

  SlimServerInfo *serverInfo;   // convenience pointer

  QByteArray macAddress;       // NOTE: this is stored in URL escaped form, since that is how we mostly use it.  If you need it in plain text COPY IT to another string and use QUrl::decode() on that string.
//  QString ipAddress;        // IP address of this device (i.e., the player)
  QString SlimServerAddr;   // server IP address
  quint16 cliPort;          // port to use for cli, usually 9090, but we allow the user to change this

  QString cliUsername;      // username for cli if needed
  QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!
  bool useAuthentication;   // test for using authentication
  bool isAuthenticated;     // have we been authenticated?
  QByteArray MaxRequestSize;      // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)
  int iTimeOut;             // number of milliseconds before CLI blocking requests time out

  bool maintainConnection;  // do we reconnect if the connection is ended (set to false when shutting down so we don't attempt a reconnect
  bool bConnection;         // is connected?

  cliState myCliState;       // state of cli / setup process

  void DeviceMsgProcessing( void ); // messages forwarded to devices
  void SystemMsgProcessing( void ); // messages forwarded to the system for processing

  bool SetupLogin( void );
//  bool SetupDevices( void );
//  void InitDevices( void );
//  bool GetDatabaseInfo(void);

  void ProcessLoginMsg( void );
  void ProcessControlMsg( void );
  void processStatusMsg( void );
  bool waitForResponse( void );

private slots:
  bool msgWaiting( void );          // we have a message waiting from the server
  bool CLIConnectionOpen( void );   // CLI interface successfully established
  void LostConnection( void );      // lost connection (check if we want reconnect)
  void ConnectionError( int err );  // error message sent with connection
  void ConnectionError( QAbstractSocket::SocketError );
  void SentBytes( int b );          // bytes sent to SqueezeCenter
};


#endif // SLIMCLI_H
