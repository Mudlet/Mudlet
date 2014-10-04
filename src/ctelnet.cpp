/***************************************************************************
 *   Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com            *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "ctelnet.h"


#include "dlgComposer.h"
#include "dlgMapper.h"
#include "glwidget.h"
#include "Host.h"
#include "mudlet.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TMap.h"

#include "pre_guard.h"
#include <QDebug>
#include <QDir>
#include <QNetworkAccessManager>
#include <QProgressDialog>
#include <QStringBuilder>
#include <QTextCodec>
#include <QTimer>
#include "post_guard.h"

#include <iostream>
#include <memory>
#include <sstream>

#include <sys/types.h>
#include <stdio.h>
#include <time.h>


#ifdef DEBUG
    #undef DEBUG
#endif

//#ifdef QT_DEBUG
//    #define DEBUG
//#endif



#define DEBUG

using namespace std;



cTelnet::cTelnet( Host * pH )
: mResponseProcessed( true )
, mAlertOnNewData( true )
, mGA_Driver( false )
, mFORCE_GA_OFF( false )
, mpComposer( 0 )
, mpHost(pH)
, mpPostingTimer( new QTimer( this ) )
, mUSE_IRE_DRIVER_BUGFIX( false )
, mLF_ON_GA( false )
, mCommands( 0 )
, mMCCP_version_1( false )
, mMCCP_version_2( false )
, enableATCP( false )
, enableGMCP( false )
, enableChannel102( false )
, mIsReplaying( false )
{
    mIsTimerPosting = false;
    mNeedDecompression = false;
    mWaitingForCompressedStreamToStart = false;
    // initialize default encoding
    encoding = "UTF-8";
    encodingChanged(encoding);
    termType = QString("Mudlet %1").arg(APP_VERSION);
    if( QByteArray(APP_BUILD).trimmed().length() )
        termType.append( QString(APP_BUILD) );

    iac = iac2 = insb = false;

    command = "";
    curX = 80;
    curY = 25;

    // initialize the socket
    connect(&socket, SIGNAL(connected()), this, SLOT(handle_socket_signal_connected()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(handle_socket_signal_disconnected()));
    //connect(&socket, SIGNAL(error()), this, SLOT (handle_socket_signal_error()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT (handle_socket_signal_readyRead()));
    //connect(&socket, SIGNAL(hostFound()), this, SLOT (handle_socket_signal_hostFound()));

    // initialize telnet session
    reset();

    mpPostingTimer->setInterval( 300 );//FIXME
    connect(mpPostingTimer, SIGNAL(timeout()), this, SLOT(slot_timerPosting()));

    mTimerLogin = new QTimer( this );
    mTimerLogin->setSingleShot(true);
    connect(mTimerLogin, SIGNAL(timeout()), this, SLOT(slot_send_login()));

    mTimerPass = new QTimer( this );
    mTimerPass->setSingleShot( true );
    connect(mTimerPass, SIGNAL(timeout()), this, SLOT(slot_send_pass()));

    mpDownloader = new QNetworkAccessManager( this );
    connect(mpDownloader, SIGNAL(finished(QNetworkReply*)),this, SLOT(replyFinished(QNetworkReply*)));
}

void cTelnet::reset ()
{
    //prepare option variables
    for (int i = 0; i < 256; i++)
    {
        myOptionState[i] = false;
        hisOptionState[i] = false;
        announcedState[i] = false;
        heAnnouncedState[i] = false;
        triedToEnable[i] = false;
    }
    iac = iac2 = insb = false;
    command = "";
    mMudData = "";

}


cTelnet::~cTelnet()
{
    if(messageStack.size())
    {
        qWarning("cTelnet::~cTelnet() Instance being destroyed before it could display some messages,\nmessages are:\n------------");
        foreach(QString message, messageStack)
        {
            qWarning("%s\n------------", qPrintable( message ) );
        }
    }
    socket.deleteLater();
}


void cTelnet::encodingChanged(QString encoding)
{
    qDebug() << "cTelnet::encodingChanged() called!";
    encoding = encoding;

    // unicode carries information in form of single byte characters
    // and multiple byte character sequences.
    // the encoder and the decoder maintain translation state, i.e. they need to know the preceding
    // chars to make the correct decisions when translating into unicode and vice versa

    incomingDataCodec = QTextCodec::codecForName(encoding.toLatin1().data());
    incomingDataDecoder = incomingDataCodec->makeDecoder();

    outgoingDataCodec = QTextCodec::codecForName(encoding.toLatin1().data());
    outgoingDataDecoder = outgoingDataCodec->makeEncoder();
}


void cTelnet::connectIt(const QString &address, int port)
{
    // wird an dieser Stelle gesetzt
    if( mpHost )
    {
        mUSE_IRE_DRIVER_BUGFIX = mpHost->mUSE_IRE_DRIVER_BUGFIX;
        mLF_ON_GA = mpHost->mLF_ON_GA;
        mFORCE_GA_OFF = mpHost->mFORCE_GA_OFF;
    }

    if( socket.state() != QAbstractSocket::UnconnectedState )
    {
        socket.abort();
        connectIt( address, port );
        return;
    }

    hostName = address;
    hostPort = port;
    // QChar(0x2714));//'?'
    // QChar(0x2718));//'?'
    // QChar(0x24d8));//info i im kreis
    QString server = "[ INFO ]  - Looking up the IP address of server:" + address + ":" + QString::number(port) + " ...";
    postMessage( server );
    QHostInfo::lookupHost(address, this, SLOT(handle_socket_signal_hostFound(QHostInfo)));
}


void cTelnet::disconnect ()
{
    socket.disconnectFromHost();
    TEvent me;
    me.mArgumentList.append( "sysDisconnectionEvent" );
    me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    mpHost->raiseEvent( me );

}

void cTelnet::handle_socket_signal_error()
{
    QString err = "[ ERROR ] - TCP/IP socket ERROR:" % socket.errorString();
    postMessage( err );
}

void cTelnet::slot_send_login()
{
    sendData( mpHost->getLogin() );
}

void cTelnet::slot_send_pass()
{
    sendData( mpHost->getPass() );
}

void cTelnet::handle_socket_signal_connected()
{
    reset();
    QString msg = "[ INFO ]  - A connection has been established successfully.\n    \n    ";
    postMessage( msg );
    QString func = "onConnect";
    QString nothing = "";
    mpHost->mLuaInterpreter.call(func, nothing );
    mConnectionTime.start();
    if( (mpHost->getLogin().size()>0) && (mpHost->getPass().size()>0))
        mTimerLogin->start(2000);
    if( (mpHost->getPass().size()>0)  && (mpHost->getPass().size()>0))
        mTimerPass->start(3000);
    //sendTelnetOption(252,3);// try to force GA by telling the server that we are NOT willing to supress GA signals
    TEvent me;
    me.mArgumentList.append( "sysConnectionEvent" );
    me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    mpHost->raiseEvent( me );

}

void cTelnet::handle_socket_signal_disconnected()
{
    postData();
    TEvent me;
    me.mArgumentList.append( "sysDisconnectionEvent" );
    me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    mpHost->raiseEvent( me );
    QString msg;
    QTime timeDiff(0,0,0,0);
    msg = QString("[ INFO ]  - Connection time: %1\n    ").arg(timeDiff.addMSecs(mConnectionTime.elapsed()).toString("hh:mm:ss.zzz"));
    mNeedDecompression = false;
    reset();
    QString err =    "[ ALERT ] - Socket got disconnected.\nReason: " % socket.errorString();
    QString spacer = "    ";
    if( ! mpHost->mIsGoingDown )
    {
        postMessage( spacer );
        postMessage( err );
        postMessage( msg );
    }
}

void cTelnet::handle_socket_signal_hostFound(QHostInfo hostInfo)
{
    if(!hostInfo.addresses().isEmpty())
    {
        mHostAddress = hostInfo.addresses().first();
        QString msg = "[ INFO ]  - The IP address of "+hostName+" has been found. It is: "+mHostAddress.toString()+"\n";
        postMessage( msg );
        msg = "[ INFO ]  - Trying to connect to "+mHostAddress.toString()+":"+QString::number(hostPort)+" ...\n";
        postMessage( msg );
        socket.connectToHost(mHostAddress, hostPort);
    }
    else
    {
        socket.connectToHost(hostInfo.hostName(), hostPort);
        QString msg = "[ ERROR ] - Host name lookup Failure!\nConnection cannot be established.\nThe server name is not correct, not working properly,\nor your nameservers are not working properly.";
        postMessage( msg );
        return;
    }
}

bool cTelnet::sendData( QString & data )
{
    while( data.indexOf("\n") != -1 )
    {
        data.replace(QChar('\n'),"");
    }
    TEvent pE;
    pE.mArgumentList.append( "sysDataSendRequest" );
    pE.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    pE.mArgumentList.append( data );
    pE.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
    mpHost->raiseEvent( pE );
    if( mpHost->mAllowToSendCommand )
    {
        string outdata = (outgoingDataCodec->fromUnicode(data)).data();
        if( ! mpHost->mUSE_UNIX_EOL )
        {
            outdata.append("\r\n");
        }
        else
            outdata += "\n";
        return socketOutRaw( outdata );
    }
    else
    {
        mpHost->mAllowToSendCommand = true;
        return false;
    }
}


bool cTelnet::socketOutRaw(string & data)
{
    if( ! socket.isWritable() )
    {
        //qDebug()<<"MUDLET SOCKET ERROR: socket not connected, but wants to send data="<<data.c_str();
        return false;
    }
    int dataLength = data.length();
    int remlen = dataLength;

    /*cout << "SOCKET OUT RAW: [ ";
    for(unsigned int i=0;i<data.size();i++)
    {
        unsigned char c = data[i];
        int ci = 0;
        ci = (int)c;
        cout << "<" << ci << "> ";
    }
    cout << " ]" << endl;*/

    do
    {
        int written = socket.write(data.data(), remlen);

        if (written == -1)
        {
            return false;
        }
        remlen -= written;
        dataLength += written;
    }
    while(remlen > 0);

    if( mGA_Driver )
    {
        mCommands++;
        if( mCommands == 1 )
        {
            mWaitingForResponse = true;
            networkLatencyTime.restart();
        }
    }

    return true;
}



void cTelnet::setDisplayDimensions()
{
    int x = mpHost->mWrapAt;
    int y = mpHost->mScreenHeight;
    if(myOptionState[static_cast<int>(OPT_NAWS)])
    {
        //cout<<"TELNET: sending NAWS:"<<x<<"x"<<y<<endl;
        string s;
        s = TN_IAC;
        s += TN_SB;
        s += OPT_NAWS;
        char x1, x2, y1, y2;
        x1 = x / 256;
        x2 = x % 256;
        y1 = y / 256;
        y2 = y % 256;
        //IAC must be doubled
        s += x1;
        if(x1 == TN_IAC)
        s += TN_IAC;
        s += x2;
        if (x2 == TN_IAC)
        s += TN_IAC;
        s += y1;
        if (y1 == TN_IAC)
        s += TN_IAC;
        s += y2;
        if (y2 == TN_IAC)
        s += TN_IAC;

        s += TN_IAC;
        s += TN_SE;
        socketOutRaw(s);
    }
}

void cTelnet::sendTelnetOption( char type, char option )
{
#ifdef DEBUG_TELNET
    QString _type;
    switch ((quint8)type)
    {
    case 251: _type = "WILL"; break;
    case 252: _type = "WONT"; break;
    case 253: _type = "DO"; break;
    case 254: _type = "DONT"; break;
    default: _type = "ERROR wrong telnet type";
    };

    qDebug() << "CLIENT SENDING Telnet: "<<_type<<" "<<(quint8)option;
#endif
    string cmd;
    cmd += TN_IAC;
    cmd += type;
    cmd += option;
    socketOutRaw(cmd);
}


void cTelnet::replyFinished( QNetworkReply * reply )
{
    mpProgressDialog->close();


    QFile file( mServerPackage );
    file.open( QFile::WriteOnly );
    file.write( reply->readAll() );
    file.flush();
    file.close();
    mpHost->installPackage( mServerPackage, 0);
    QString packageName = mServerPackage.section("/",-1 );
    packageName.replace( ".zip" , "" );
    packageName.replace( "trigger", "" );
    packageName.replace( "xml", "" );
    packageName.replace( ".mpackage" , "" );
    packageName.replace( '/' , "" );
    packageName.replace( '\\' , "" );
    packageName.replace( '.' , "" );
    mpHost->mServerGUI_Package_name = packageName;
}

void cTelnet::setDownloadProgress( qint64 got, qint64 tot )
{
    mpProgressDialog->setRange(0, static_cast<int>(tot) );
    mpProgressDialog->setValue(static_cast<int>(got));
}

void cTelnet::processTelnetCommand( const string & command )
{
  char ch = command[1];
#ifdef DEBUG_TELNET
  QString _type;
  switch ((quint8)ch)
  {
  case 239: _type = "TN_EOR"; break;
  case 249: _type = "TN_GA"; break;
  case 250: _type = "SB"; break;
  case 251: _type = "WILL"; break;
  case 252: _type = "WONT"; break;
  case 253: _type = "DO"; break;
  case 254: _type = "DONT"; break;
  case 255: _type = "IAC"; break;
  default: _type = QString::number((quint8)ch);
  };
  if (command.size() > 2) {
    qDebug()<<"SERVER sends telnet signal ("<< command.size() << "):" <<_type << " + " << (quint8)command[2];
  } else {
    qDebug()<<"SERVER sends telnet signal ("<< command.size() << "):" <<_type;
  }
#endif

  char option;
  switch( ch )
  {
      case TN_GA:
      case TN_EOR:
      {
          recvdGA = true;
          break;
      }
      case TN_WILL:
      {
          //server wants to enable some option (or he sends a timing-mark)...
          option = command[2];
          int idxOption = static_cast<int>(option);

          if( option == static_cast<char>(25) ) //EOR support (END OF RECORD=TN_GA
          {
              qDebug() << "EOR enabled";
              sendTelnetOption( TN_DO, 25 );
              break;
          }

          if( option == MSDP ) //MSDP support
          {
              string _h;
              if( !mpHost->mEnableMSDP ) {
                  _h += TN_IAC;
                  _h += TN_DONT;
                  _h += MSDP; // disable MSDP per http://tintin.sourceforge.net/msdp/
                  socketOutRaw( _h );
                  qDebug() << "TELNET IAC DONT MSDP";
                  break;
              } else {
                  sendTelnetOption( TN_DO, 69 );
                  //need to send MSDP start sequence: IAC   SB MSDP MSDP_VAR "LIST" MSDP_VAL "COMMANDS" IAC SE
                  //NOTE: MSDP does not need quotes for string/vals
                  _h += TN_IAC;
                  _h += TN_SB;
                  _h += MSDP; //MSDP
                  _h += 1; //MSDP_VAR
                  _h += "LIST";
                  _h += 2; //MSDP_VAL
                  _h += "COMMANDS";
                  _h += TN_IAC;
                  _h += TN_SE;
                  socketOutRaw( _h );
                  qDebug() << "TELNET IAC DO MSDP";
                  break;
              }
          }
          if( option == static_cast<char>(200) ) // ATCP support
          {
              //FIXME: this is a bug, some muds offer both atcp + gmcp
              if( mpHost->mEnableGMCP ) break;

              qDebug() << "ATCP enabled";
              enableATCP = true;
              sendTelnetOption( TN_DO, 200 );

              string _h;
              _h += TN_IAC;
              _h += TN_SB;
              _h += 200;
              _h += string("hello Mudlet ") + APP_VERSION + APP_BUILD + string("\ncomposer 1\nchar_vitals 1\nroom_brief 1\nroom_exits 1\nmap_display 1\n");
              _h += TN_IAC;
              _h += TN_SE;
              socketOutRaw( _h );
              break;
          }

          if( option == GMCP )
          {
              if( !mpHost->mEnableGMCP ) break;

              enableGMCP = true;
              sendTelnetOption( TN_DO, GMCP );
              qDebug() << "GMCP enabled";

              string _h;
              _h = TN_IAC;
              _h += TN_SB;
              _h += GMCP;
              _h += string("Core.Hello { \"client\": \"Mudlet\", \"version\": \"") + APP_VERSION + APP_BUILD + string("\" }");
              _h += TN_IAC;
              _h += TN_SE;

              socketOutRaw( _h );

              _h = TN_IAC;
              _h += TN_SB;
              _h += GMCP;
              _h += "Core.Supports.Set [ \"Char 1\", \"Char.Skills 1\", \"Char.Items 1\", \"Room 1\", \"IRE.Rift 1\", \"IRE.Composer 1\"]";
              _h += TN_IAC;
              _h += TN_SE;

              socketOutRaw( _h );
              break;
          }

          if( option == MXP )
          {
              if( ! mpHost->mFORCE_MXP_NEGOTIATION_OFF )
              {
                sendTelnetOption( TN_DO, 91 );
                //mpHost->mpConsole->print("\n<MXP enabled>\n");
                break;
              }
              //else
                  //mpHost->mpConsole->print("\n<MXP declined because of user setting: force MXP off>");
          }

          //option = command[2];
          if( option == static_cast<char>(102) ) // Aardwulf channel 102 support
          {
              qDebug() << "Aardwulf channel 102 support enabled";
              enableChannel102 = true;
              sendTelnetOption( TN_DO, 102 );
              break;
          }

          heAnnouncedState[idxOption] = true;
          if( triedToEnable[idxOption] )
          {
              hisOptionState[idxOption] = true;
              triedToEnable[idxOption] = false;
          }
          else
          {
              if( !hisOptionState[idxOption] )
              {
                   //only if this is not set; if it's set, something's wrong wth the server
                   //(according to telnet specification, option announcement may not be
                   //unless explicitly requested)

                   if( //( option == OPT_SUPPRESS_GA ) ||
                       ( option == OPT_STATUS ) ||
                       ( option == OPT_TERMINAL_TYPE) ||
                       ( option == OPT_ECHO ) ||
                       ( option == OPT_NAWS ) )
                   {
                       sendTelnetOption( TN_DO, option );
                       hisOptionState[idxOption] = true;
                   }
                   else if( ( option == OPT_COMPRESS ) || ( option == OPT_COMPRESS2 ) )
                   {
                       //these are handled separately, as they're a bit special
                       if( mpHost->mFORCE_NO_COMPRESSION || ( ( option == OPT_COMPRESS ) && ( hisOptionState[static_cast<int>(OPT_COMPRESS2)] ) ) )
                       {
                           //protocol says: reject MCCP v1 if you have previously accepted
                           //MCCP v2...
                           sendTelnetOption( TN_DONT, option );
                           hisOptionState[idxOption] = false;
                           qDebug() << "Rejecting MCCP v1, because v2 has already been negotiated.";
                       }
                       else
                       {
                           sendTelnetOption( TN_DO, option );
                           hisOptionState[idxOption] = true;
                           //inform MCCP object about the change
                           if( option == OPT_COMPRESS )
                           {
                               mMCCP_version_1 = true;
                               //MCCP->setMCCP1(true);
                               qDebug() << "MCCP v1 negotiated.";
                           }
                           else
                           {
                               mMCCP_version_2 = true;
                               //MCCP->setMCCP2( true );
                               qDebug() << "MCCP v2 negotiated!";
                           }
                       }
                   }
                   else if( supportedTelnetOptions.contains( option ) )
                   {
                       sendTelnetOption( TN_DO, option );
                       hisOptionState[idxOption] = true;
                   }
                   else
                   {
                       sendTelnetOption( TN_DONT, option );
                       hisOptionState[idxOption] = false;
                   }
               }
          }


          break;
      }

      case TN_WONT:
      {

          //server refuses to enable some option...
          #ifdef DEBUG
              qDebug() << "cTelnet::processTelnetCommand() TN_WONT command="<<(quint8)command[2];
          #endif
          option = command[2];
          int idxOption = static_cast<int>(option);
          if( triedToEnable[idxOption] )
          {
              hisOptionState[idxOption] = false;
              triedToEnable[idxOption] = false;
              heAnnouncedState[idxOption] = true;
          }
          else
          {
              #ifdef DEBUG
                  qDebug() << "cTelnet::processTelnetCommand() we dont accept his option because we didnt want it to be enabled";
              #endif
              //send DONT if needed (see RFC 854 for details)
              if( hisOptionState[idxOption] || ( heAnnouncedState[idxOption] ) )
              {
                  sendTelnetOption( TN_DONT, option );
                  hisOptionState[idxOption] = false;

                  if( option == OPT_COMPRESS )
                  {
                      //MCCP->setMCCP1 (false);
                      mMCCP_version_1 = false;
                      qDebug() << "MCCP v1 disabled !";
                  }
                  if( option == OPT_COMPRESS2 )
                  {
                      mMCCP_version_2 = false;
                      //MCCP->setMCCP2 (false);
                      qDebug() << "MCCP v1 disabled !";
                  }
              }
              heAnnouncedState[idxOption] = true;
          }
          break;
      }

      case TN_DO:
      {
#ifdef DEBUG
      qDebug() << "telnet: server wants us to enable option:"<< (quint8)command[2];
#endif
          //server wants us to enable some option
          option = command[2];
          int idxOption = static_cast<int>(option);
          if( option == static_cast<char>(69) && mpHost->mEnableMSDP ) // MSDP support
          {
            qDebug() << "TELNET IAC DO MSDP";
            sendTelnetOption( TN_WILL, 69 );

            break;
          }
          if( option == static_cast<char>(200) && !mpHost->mEnableGMCP ) // ATCP support, enable only if GMCP is off as GMCP is better
          {
            qDebug() << "TELNET IAC DO ATCP";
            enableATCP = true;
            sendTelnetOption( TN_WILL, 200 );
            break;
          }
          if( option == static_cast<char>(201) ) // GMCP support
          {
            qDebug() << "TELNET IAC DO GMCP";
            enableATCP = true;
            sendTelnetOption( TN_WILL, 201 );
            break;
          }
          if( option == MXP ) // MXP support
          {
            sendTelnetOption( TN_WILL, 91 );
            mpHost->mpConsole->print("\n<MXP support enabled>\n");
            break;
          }
          if( option == static_cast<char>(102) ) // channel 102 support
          {
            qDebug() << "TELNET IAC DO CHANNEL 102";
            enableChannel102 = true;
            sendTelnetOption( TN_WILL, 102 );
            break;
          }
#ifdef DEBUG
          qDebug() << "server wants us to enable telnet option " << (quint8)option << "(TN_DO + "<< (quint8)option<<")";
#endif
          if(option == OPT_TIMING_MARK)
          {
              qDebug() << "OK we are willing to enable TIMING_MARK";
              //send WILL TIMING_MARK
              sendTelnetOption( TN_WILL, option );
          }
          else if (!myOptionState[255])
          //only if the option is currently disabled
          {
              if( //( option == OPT_SUPPRESS_GA ) ||
                  ( option == OPT_STATUS ) ||
                  ( option == OPT_NAWS ) ||
                  ( option == OPT_TERMINAL_TYPE ) )
              {
                  if( option == OPT_SUPPRESS_GA ) qDebug() << "OK we are willing to enable option SUPPRESS_GA";
                  if( option == OPT_STATUS ) qDebug() << "OK we are willing to enable telnet option STATUS";
                  if( option == OPT_TERMINAL_TYPE ) qDebug() << "OK we are willing to enable telnet option TERMINAL_TYPE";
                  if( option == OPT_NAWS ) qDebug() << "OK we are willing to enable telnet option NAWS";
                  sendTelnetOption( TN_WILL, option );
                  myOptionState[idxOption] = true;
                  announcedState[idxOption] = true;
              }
              else
              {
                  qDebug() << "SORRY, we are NOT WILLING to enable this telnet option.";
                  sendTelnetOption (TN_WONT, option);
                  myOptionState[idxOption] = false;
                  announcedState[idxOption] = true;
              }
          }
          if( option == OPT_NAWS )
          {
              //NAWS
              setDisplayDimensions();
          }
          break;
      }
      case TN_DONT:
      {
          //only respond if value changed or if this option has not been announced yet
#ifdef DEBUG
              qDebug() << "cTelnet::processTelnetCommand() TN_DONT command="<<(quint8)command[2];
#endif
          option = command[2];
          int idxOption = static_cast<int>(option);
          if( myOptionState[idxOption] || ( !announcedState[idxOption] ) )
          {
              sendTelnetOption (TN_WONT, option);
              announcedState[idxOption] = true;
          }
          myOptionState[idxOption] = false;
          break;
      }
      case TN_SB:
      {
          option = command[2];

          // MSDP
          if( option == static_cast<char>(69) )
          {
              QString _m = command.c_str();
              if( command.size() < 6 ) return;
              _m = _m.mid( 3, command.size()-5 );
              mpHost->mLuaInterpreter.msdp2Lua( _m.toLocal8Bit().data(), _m.length() );
              return;
          }
          // ATCP
          if( option == static_cast<char>(200) )
          {
              QString _m = command.c_str();
              if( command.size() < 6 ) return;
              _m = _m.mid( 3, command.size()-5 );
              setATCPVariables( _m );
              if( _m.startsWith("Auth.Request") )
              {
                  string _h;
                  _h += TN_IAC;
                  _h += TN_SB;
                  _h += 200;
                  _h += string("hello Mudlet ") + APP_VERSION + APP_BUILD + string("\ncomposer 1\nchar_vitals 1\nroom_brief 1\nroom_exits 1\n");
                  _h += TN_IAC;
                  _h += TN_SE;
                  socketOutRaw( _h );
              }

              if( _m.startsWith( "Client.GUI" ) )
              {
                  if( ! mpHost->mAcceptServerGUI ) return;

                  QString version = _m.section( '\n', 0 );
                  version.replace("Client.GUI ", "");
                  version.replace('\n', " ");
                  version = version.section(' ', 0, 0);

                  int newVersion = version.toInt();
                  //QString __mkp = QString("<old version:'%1' new version:'%2' name:'%3' msg:'%4'>\n").arg(mpHost->mServerGUI_Package_version).arg(newVersion).arg(mpHost->mServerGUI_Package_name).arg(version);
                  //mpHost->mpConsole->print(__mkp);
                  if( mpHost->mServerGUI_Package_version != newVersion )
                  {
                      QString _smsg = QString("<The server wants to upgrade the GUI to new version '%1'. Uninstalling old version '%2'>").arg(mpHost->mServerGUI_Package_version).arg(newVersion);
                      mpHost->mpConsole->print(_smsg.toLatin1().data());
                      mpHost->uninstallPackage( mpHost->mServerGUI_Package_name, 0);
                      mpHost->mServerGUI_Package_version = newVersion;
                  }
                  QString url = _m.section( '\n', 1 );
                  QString packageName = url.section('/',-1);
                  QString fileName = packageName;
                  packageName.replace( ".zip" , "" );
                  packageName.replace( "trigger", "" );
                  packageName.replace( "xml", "" );
                  packageName.replace( ".mpackage" , "" );
                  packageName.replace( '/' , "" );
                  packageName.replace( '\\' , "" );
                  packageName.replace( '.' , "" );
                  mpHost->mpConsole->print("<Server offers downloadable GUI (url='");
                  mpHost->mpConsole->print( url );
                  mpHost->mpConsole->print("') (package='");
                  mpHost->mpConsole->print(packageName);
                  mpHost->mpConsole->print("')>\n");
                  if( mpHost->mInstalledPackages.contains( packageName ) )
                  {
                      mpHost->mpConsole->print("<package is already installed>\n");
                      return;
                  }
                  QString _home = QDir::homePath();
                  _home.append( "/.config/mudlet/profiles/" );
                  _home.append( mpHost->getName() );
                  mServerPackage = QString( "%1/%2").arg( _home ).arg( fileName );

                  QNetworkReply * reply = mpDownloader->get( QNetworkRequest( QUrl( url ) ) );
                  mpProgressDialog = new QProgressDialog("downloading game GUI from server", "Abort", 0, 4000000, mpHost->mpConsole );
                  connect(reply, SIGNAL(downloadProgress( qint64, qint64 )), this, SLOT(setDownloadProgress(qint64,qint64)));
                  mpProgressDialog->show();

              }
              return;
          }

          // GMCP
          if( option == static_cast<char>(201) )
          {
              QString _m = command.c_str();
              if( command.size() < 6 ) return;
              _m = _m.mid( 3, command.size()-5 );
              setGMCPVariables( _m );
              return;
          }

          if( option == static_cast<unsigned char>(102) )
          {
              QString _m = command.c_str();
              if( command.size() < 6 ) return;
              _m = _m.mid( 3, command.size()-5 );
              setChannel102Variables( _m );
              return;
          }
          switch( option ) //switch 2
          {
              case OPT_STATUS:
              {
                  //see OPT_TERMINAL_TYPE for explanation why I'm doing this
                  if( true )
                  {
                      qDebug() << "WARNING: FIXME #501";
                      if(command[3] == TNSB_SEND)
                      {
                          qDebug() << "WARNING: FIXME #504";
                          //request to send all enabled commands; if server sends his
                          //own list of commands, we just ignore it (well, he shouldn't
                          //send anything, as we do not request anything, but there are
                          //so many servers out there, that you can never be sure...)
                          string cmd;
                          cmd +=  TN_IAC;
                          cmd +=  TN_SB;
                          cmd +=  OPT_STATUS;
                          cmd +=  TNSB_IS;
                          for (short i = 0; i < 256; i++)
                          {
                              if (myOptionState[i])
                              {
                                  cmd +=  TN_WILL;
                                  cmd +=  i;
                              }
                              if (hisOptionState[i])
                              {
                                  cmd +=  TN_DO;
                                  cmd +=  i;
                              }
                          }
                          cmd +=  TN_IAC;
                          cmd +=  TN_SE;
                          socketOutRaw(cmd);
                      }
                  }
                  break;
              }

              case OPT_TERMINAL_TYPE:
              {
                  qDebug() << "server sends telnet option terminal type";
                  if( myOptionState[static_cast<int>(OPT_TERMINAL_TYPE)] )
                  {
                      if(command[3] == TNSB_SEND )
                      {
                           //server wants us to send terminal type; he can send his own type
                           //too, but we just ignore it, as we have no use for it...
                           string cmd;
                           cmd +=  TN_IAC;
                           cmd +=  TN_SB;
                           cmd +=  OPT_TERMINAL_TYPE;
                           cmd +=  TNSB_IS;
                           cmd += termType.toLatin1().data();
                           cmd +=  TN_IAC;
                           cmd +=  TN_SE;
                           socketOutRaw( cmd );
                      }
                  }
                  //other cmds should not arrive, as they were not negotiated.
                  //if they do, they are merely ignored
              }
          };//end switch 2
          //other commands are simply ignored (NOP and such, see .h file for list)
      }
  };//end switch 1
  // raise sysTelnetEvent for all unhandled protocols
  // EXCEPT TN_GA (performance)
  if( command[1] != TN_GA )
  {
      unsigned char type = static_cast<unsigned char>(command[1]);
      unsigned char telnetOption = static_cast<unsigned char>(command[2]);
      QString msg = command.c_str();
      if( command.size() >= 6 ) msg = msg.mid( 3, command.size()-5 );
      TEvent me;
      me.mArgumentList.append( "sysTelnetEvent" );
      me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
      me.mArgumentList.append( QString::number(type) );
      me.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
      me.mArgumentList.append( QString::number(telnetOption) );
      me.mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
      me.mArgumentList.append( msg );
      me.mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
      mpHost->raiseEvent( me );
  }
}

void cTelnet::setATCPVariables(const QString & msg )
{
    QString var;
    QString arg;
    bool single = true;
    if( msg.indexOf( '\n' ) > -1 )
    {
        var = msg.section( "\n", 0, 0 );
        arg = msg.section( "\n", 1 );
        single = false;
    }
    else
    {
        var = msg.section( " ", 0, 0 );
        arg = msg.section( " ", 1 );
    }

    if( var.startsWith("Client.Compose") )
    {
        QString title;
        if( ! single )
            title = var.section( " ", 1 );
        else
        {
            title = arg;
            arg = "";
        }
        if( mpComposer )
        {
            return;
        }
        mpComposer = new dlgComposer( mpHost );
        //FIXME
        if( arg.startsWith(" ") )
        {
            arg.remove(0,1);
        }
        mpComposer->init( title, arg );
        mpComposer->raise();
        mpComposer->show();
        return;
    }
    var.remove( '.' );
    arg.remove( '\n' );
    int space = var.indexOf( ' ' );
    if( space > -1 )
    {
        arg.prepend(" ");
        arg = arg.prepend( var.section( " ", 1 ) );
        var = var.section( " ", 0, 0 );
    }
    mpHost->mLuaInterpreter.setAtcpTable( var, arg );
    if( var.startsWith("RoomNum") )
    {
        if( mpHost->mpMap )
        {
            mpHost->mpMap->mRoomId = arg.toInt();
            if( mpHost->mpMap->mpM && mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap )
            {
                mpHost->mpMap->mpM->update();
                mpHost->mpMap->mpMapper->mp2dMap->update();
            }
        }
    }
}

void cTelnet::setGMCPVariables(const QString & msg )
{
    QString var;
    QString arg;
// N/U:    bool single = true;
    if( msg.indexOf( '\n' ) > -1 )
    {
        var = msg.section( "\n", 0, 0 );
        arg = msg.section( "\n", 1 );
// N/U:        single = false;
    }
    else
    {
        var = msg.section( " ", 0, 0 );
        arg = msg.section( " ", 1 );
    }

    //printf("message: '%s', body: '%s'\n", var.toLatin1().data(), arg.toLatin1().data());
    if( msg.startsWith( "Client.GUI" ) )
    {
        if( ! mpHost->mAcceptServerGUI ) return;

        QString version = msg.section( '\n', 0 );
        version.replace("Client.GUI ", "");
        version.replace('\n', " ");
        version = version.section(' ', 0, 0);

        int newVersion = version.toInt();
        //QString __mkp = QString("<old version:'%1' new version:'%2' name:'%3' msg:'%4'>\n").arg(mpHost->mServerGUI_Package_version).arg(newVersion).arg(mpHost->mServerGUI_Package_name).arg(version);
        //mpHost->mpConsole->print(__mkp);
        if( mpHost->mServerGUI_Package_version != newVersion )
        {
            QString _smsg = QString("<The server wants to upgrade the GUI to new version '%1'. Uninstalling old version '%2'>").arg(mpHost->mServerGUI_Package_version).arg(newVersion);
            mpHost->mpConsole->print(_smsg.toLatin1().data());
            mpHost->uninstallPackage( mpHost->mServerGUI_Package_name, 0);
            mpHost->mServerGUI_Package_version = newVersion;
        }
        QString url = msg.section( '\n', 1 );
        QString packageName = url.section('/',-1);
        QString fileName = packageName;
        packageName.replace( ".zip" , "" );
        packageName.replace( "trigger", "" );
        packageName.replace( "xml", "" );
        packageName.replace( ".mpackage" , "" );
        packageName.replace( '/' , "" );
        packageName.replace( '\\' , "" );
        packageName.replace( '.' , "" );
        mpHost->mpConsole->print("<Server offers downloadable GUI (url='");
        mpHost->mpConsole->print( url );
        mpHost->mpConsole->print("') (package='");
        mpHost->mpConsole->print(packageName);
        mpHost->mpConsole->print("')>\n");
        if( mpHost->mInstalledPackages.contains( packageName ) )
        {
            mpHost->mpConsole->print("<package is already installed>\n");
            return;
        }
        QString _home = QDir::homePath();
        _home.append( "/.config/mudlet/profiles/" );
        _home.append( mpHost->getName() );
        mServerPackage = QString( "%1/%2").arg( _home ).arg( fileName );

        QNetworkReply * reply = mpDownloader->get( QNetworkRequest( QUrl( url ) ) );
        mpProgressDialog = new QProgressDialog("downloading game GUI from server", "Abort", 0, 4000000, mpHost->mpConsole );
        connect(reply, SIGNAL(downloadProgress( qint64, qint64 )), this, SLOT(setDownloadProgress(qint64,qint64)));
        mpProgressDialog->show();
        return;
    }
    arg.remove( '\n' );
    // remove \r's from the data, as yajl doesn't like it
    arg.remove(QChar('\r'));
    mpHost->mLuaInterpreter.setGMCPTable( var, arg );
}

void cTelnet::setChannel102Variables(const QString & msg )
{
    // messages consist of 2 bytes only
    if( msg.size() < 2 )
    {
        qDebug()<<"ERROR: channel 102 message size != 2 bytes msg<"<<msg<<">";
        return;
    }
    else
    {
        int _m = msg.at(0).toLatin1();
        int _a = msg.at(1).toLatin1();
        mpHost->mLuaInterpreter.setChannel102Table( _m, _a );
    }
}

void cTelnet::atcpComposerCancel()
{
    if( ! mpComposer ) return;
    mpComposer->close();
    mpComposer = 0;
    string msg = "*q\nno\n";
    socketOutRaw( msg );
}

void cTelnet::atcpComposerSave( QString txt )
{
    if( ! mpHost->mEnableGMCP )
    {
        //olesetbuf \n <text>
        string _h;
        _h += TN_IAC;
        _h += TN_SB;
        _h += 200;
        _h += "olesetbuf \n ";
        _h += txt.toLatin1().data();
        _h += '\n';
        _h += TN_IAC;
        _h += TN_SE;
        socketOutRaw( _h );
        _h.clear();
        _h += "*s\n";
        socketOutRaw( _h );
    }
    else
    {
        string _h;
        _h += TN_IAC;
        _h += TN_SB;
        _h += GMCP;
        _h += "IRE.Composer.SetBuffer";
        if( txt != "" )
        {
            _h += "  ";
            _h += txt.toLatin1().data();
            _h += " ";
        }
        _h += TN_IAC;
        _h += TN_SE;
        socketOutRaw( _h );
        _h.clear();
        _h += "*s\n";
        socketOutRaw( _h );
    }
    if( ! mpComposer ) return;
    mpComposer->close();
    mpComposer = 0;
}

/*string cTelnet::getCurrentTime()
{
    time_t t;
    time(&t);
    tm lt;
    ostringstream s;
    s.str("");
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    localtime_r( &t, &lt );
    s << "["<<lt.tm_hour<<":"<<lt.tm_min<<":"<<lt.tm_sec<<":"<<tv.tv_usec<<"]";
    string time = s.str();
    return time;
} */


// Revamped to take additional [ WARN ], [ ALERT ] and [ INFO ] prefixes and to indent
// additional lines (ending with '\n') to last space character after "-"
// following prefix.
// Prefixes are made uppercase.
// Will store messages if the TConsole on which they are to be placed is not yet
// in existance as happens during startup, then pumps them out in order of
// arrival once a message arrives when the TConsole DOES exist.
void cTelnet::postMessage( QString msg )
{
    messageStack.append(msg);

    if( ! mpHost->mpConsole )
    {
        // Console doesn't exist (yet), stack up messages until it does...
        return;
    }

    while(messageStack.size())
    {
        while( messageStack.first().endsWith('\n') )
        { // Must strip off final line feeds as use that character for split() - will replace it later
            messageStack.first().chop(1);
        }

        QStringList body = messageStack.first().split(QChar('\n'));

        qint8 openBraceIndex = body.at(0).indexOf("[");
        qint8 closeBraceIndex = body.at(0).indexOf("]");
        qint8 hyphenIndex = body.at(0).indexOf("- ");
        if( openBraceIndex >= 0 && closeBraceIndex > 0 && closeBraceIndex < hyphenIndex )
        {
            quint8 prefixLength = hyphenIndex + 1;
            while( body.at(0).at(prefixLength) == ' ' )
            {
                prefixLength++;
            }

            QString prefix = body.at(0).left(prefixLength).toUpper();
            QString firstLineTail = body.at(0).mid(prefixLength);
            body.removeFirst();
            if( prefix.contains("ERROR") )
            {
                mpHost->mpConsole->print( prefix, 150, 0, 0, 0, 0, 0 ); // Red on black
                mpHost->mpConsole->print( firstLineTail.append('\n'), 150, 0, 0, 0, 0, 0 );  // Red on black
                for( quint8 _i = 0; _i < body.size(); _i++ )
                {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    // Fix for lua using tabs for indentation which was messing up justification:
                    body[_i] = temp.rightJustified( temp.length() + prefixLength );
                }
                if( body.size() )
                    mpHost->mpConsole->print( body.join('\n').append('\n'), 150, 0, 0, 0, 0, 0 );  // Red on black
            }
            else if( prefix.contains("WARN") )
            {
                mpHost->mpConsole->print( prefix, 0, 150, 190, 0, 0, 0 );
                mpHost->mpConsole->print( firstLineTail.append('\n'), 190, 150, 0, 0, 0, 0 ); //Foreground dark grey, background bright grey
                for( quint8 _i = 0; _i < body.size(); _i++ )
                {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified( temp.length() + prefixLength );
                }
                if( body.size() )
                    mpHost->mpConsole->print( body.join('\n').append('\n'), 190, 150, 0, 0, 0, 0 );
            }
            else if( prefix.contains("ALERT") )
            {
                mpHost->mpConsole->print( prefix, 190, 100, 50, 0, 0, 0 ); // Orangish on black
                mpHost->mpConsole->print( firstLineTail.append('\n'), 190, 190, 50, 0, 0, 0 ); // Yellow on Black
                for( quint8 _i = 0; _i < body.size(); _i++ )
                {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    // Fix for lua using tabs for indentation which was messing up justification:
                    body[_i] = temp.rightJustified( temp.length() + prefixLength );
                }
                if( body.size() )
                    mpHost->mpConsole->print( body.join('\n').append('\n'), 190, 190, 50, 0, 0, 0 ); // Yellow on Black
            }
            else if( prefix.contains("INFO") )
            {
                mpHost->mpConsole->print( prefix, 0, 150, 190, 0, 0, 0 ); // Cyan on black
                mpHost->mpConsole->print( firstLineTail.append('\n'), 0, 160, 0, 0, 0, 0 );  // Light Green on Black
                for( quint8 _i = 0; _i < body.size(); _i++ )
                {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified( temp.length() + prefixLength );
                }
                if( body.size() )
                    mpHost->mpConsole->print( body.join('\n').append('\n'), 0, 160, 0, 0, 0, 0 );  // Light Green on Black
            }
            else if( prefix.contains("OK") )
            {
                mpHost->mpConsole->print( prefix, 0, 160, 0, 0, 0, 0 );  // Light Green on Black
                mpHost->mpConsole->print( firstLineTail.append('\n'), 190, 100, 50, 0, 0, 0 ); // Orangish on black
                for( quint8 _i = 0; _i < body.size(); _i++ )
                {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified( temp.length() + prefixLength );
                }
                if( body.size() )
                    mpHost->mpConsole->print( body.join('\n').append('\n'), 190, 100, 50, 0, 0, 0 ); // Orangish on black
            }
            else
            {  // Unrecognised but still in a "[ something ] -  message..." format
                mpHost->mpConsole->print( prefix, 190, 50, 50, 190, 190, 190 ); // Foreground red, background bright grey
                mpHost->mpConsole->print( firstLineTail.append('\n'), 50, 50, 50, 190, 190, 190 ); //Foreground dark grey, background bright grey
                for( quint8 _i = 0; _i < body.size(); _i++ )
                {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified( temp.length() + prefixLength );
                }
                if( body.size() )
                    mpHost->mpConsole->print( body.join('\n').append('\n'), 50, 50, 50, 190, 190, 190 ); //Foreground dark grey, background bright grey
            }
        }
        else
        {  // No prefix found
            mpHost->mpConsole->print( body.join('\n').append('\n'), 190, 190, 190, 0, 0, 0 ); //Foreground bright grey, background black
        }
        messageStack.removeFirst();
    }
}

//forward data for further processing


void cTelnet::gotPrompt( string & mud_data )
{
    mpPostingTimer->stop();
    mMudData += mud_data;

    if( mUSE_IRE_DRIVER_BUGFIX && mGA_Driver )
    {
        //////////////////////////////////////////////////////////////////////
        //
        // Patch for servers that need GA/EOR for prompt fixups
        //

        int j = 0;
        int s = mMudData.size();
        while( j < s )
        {
            // search for leading <LF> but skip leading ANSI control sequences
            if( mMudData[j] == 0x1B )
            {
                while( j < s )
                {
                    if( mMudData[j] == 'm' )
                    {
                        goto NEXT;
                        break;
                    }
                    j++;
                }
            }
            if( mMudData[j] == '\n' )
            {
                mMudData.erase( j, 1 );
                break;
            }
            else
            {
                break;
            }
            NEXT: j++;
        }
        //
        ////////////////////////////
    }

    postData();
    mMudData = "";
    mIsTimerPosting = false;
}

void cTelnet::gotRest( string & mud_data )
{

    if( mud_data.size() < 1 )
    {
        return;
    }
    //if( ( ! mGA_Driver ) || ( mud_data[mud_data.size()-1] == '\n' ) )
    if( ! mGA_Driver )
    {
        size_t i = mud_data.rfind('\n');
        if( i != string::npos )
        {
            mMudData += mud_data.substr( 0, i+1 );
            postData();
            mpPostingTimer->start();
            mIsTimerPosting = true;
            if( i+1 < mud_data.size() )
            {
                mMudData = mud_data.substr( i+1, mud_data.size() );
            }
            else
            {
                mMudData = "";
            }
        }
        else
        {
            mMudData += mud_data;
            if( ! mIsTimerPosting )
            {
                mpPostingTimer->start();
                mIsTimerPosting = true;
            }
        }

    }
    else if( mGA_Driver )// if( ( mud_data[mud_data.size()-1] == '\n' ) )
    {

        //mpPostingTimer->stop();
        mMudData += mud_data;
        postData();
        mMudData = "";
        //mIsTimerPosting = false;
    }
    else
    {
        mMudData += mud_data;
        if( ! mIsTimerPosting )
        {
            mpPostingTimer->start();
            mIsTimerPosting = true;
        }
    }
}

void cTelnet::slot_timerPosting()
{
    if( ! mIsTimerPosting ) return;
    mMudData += "\r";
    postData();
    mMudData = "";
    mIsTimerPosting = false;
    mpHost->mpConsole->finalize();
}

void cTelnet::postData()
{
    //QString cd = incomingDataDecoder->toUnicode( mMudData.data(), mMudData.size() );
    if (mpHost->mpConsole) {
        mpHost->mpConsole->printOnDisplay(mMudData);
    }
    if( mAlertOnNewData )
    {
        QApplication::alert( mudlet::self(), 0 );
    }
}

void cTelnet::initStreamDecompressor()
{
    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;
    mZstream.avail_in = 0;
    mZstream.next_in = Z_NULL;

    inflateInit( & mZstream );
}

int cTelnet::decompressBuffer( char *& in_buffer, int& length, char* out_buffer )
{
    mZstream.avail_in = length;
    mZstream.next_in = (Bytef *) in_buffer;

    mZstream.avail_out = 100000;
    mZstream.next_out = (Bytef *) out_buffer;

    int zval = inflate( & mZstream, Z_SYNC_FLUSH );
    int outSize = 100000 - mZstream.avail_out;

    length = mZstream.avail_in;
    in_buffer = (char*)mZstream.next_in;

    if( zval == Z_STREAM_END )
    {
        inflateEnd( & mZstream );
        qDebug() << "recv Z_STREAM_END, ending compression";
        this->mNeedDecompression = false;
        this->mMCCP_version_1 = false;
        this->mMCCP_version_2 = false;

        // Reset the option state so we can re-enable compression again in the future
        // such as in the case of a copyover -JM
        hisOptionState[static_cast<int>(OPT_COMPRESS)] = false;
        hisOptionState[static_cast<int>(OPT_COMPRESS2)] = false;
    }
    else
    {
        if( zval < 0 )
        {
            initStreamDecompressor();
            return -1;
        }
    }
    return outSize;
}


void cTelnet::recordReplay()
{
    lastTimeOffset = 0;
    timeOffset.start();
}

void cTelnet::loadReplay( QString & name )
{
    mReplayFile.setFileName( name );
    QString msg = "loading replay " + name;
    postMessage( msg );
    mReplayFile.open( QIODevice::ReadOnly );
    mReplayStream.setDevice( &mReplayFile );
    mIsReplaying = true;
    mudlet::self()->replayStart();
    _loadReplay();
}

void cTelnet::_loadReplay()
{
    if( ! mReplayStream.atEnd() )
    {
        int offset;
        int amount;
        mReplayStream >> offset;
        mReplayStream >> amount;

        char * pB = &mLoadBuffer[0];
        mLoadedBytes = mReplayStream.readRawData ( pB, amount );
        qDebug("_loadReplay(): loaded: %i/%i bytes, wait for %1.3f seconds. (Single shot duration is: %1.3f seconds.)",
               mLoadedBytes,
               amount,
               offset / 1000.0,
               offset / (1000.0 * mudlet::self()->mReplaySpeed));
        mLoadBuffer[mLoadedBytes] = '\0'; // Previous use of loadedBytes + 1 caused a spurious character at end of string display by a qDebug of the loadBuffer contents
        mudlet::self()->mReplayTime = mudlet::self()->mReplayTime.addMSecs(offset);
        QTimer::singleShot(offset / mudlet::self()->mReplaySpeed, this, SLOT(readPipe()));
    }
    else
    {
        mIsReplaying = false;
        mReplayFile.close();
        QString msg = "The replay has ended.\n";
        postMessage( msg );
        mudlet::self()->replayOver();
    }
}


void cTelnet::readPipe()
{
    int datalen = mLoadedBytes;
    string cleandata = "";
    recvdGA = false;
    qDebug("Replay data: \"%s\"", mLoadBuffer);
    for( int i = 0; i < datalen; i++ )
    {
        char ch = mLoadBuffer[i];
        if( iac || iac2 || insb || (ch == TN_IAC) )
        {
            #ifdef DEBUG
                qDebug() <<" SERVER sends telnet command "<<(quint8)ch;
            #endif
            if (! (iac || iac2 || insb) && ( ch == TN_IAC ) )
            {
                iac = true;
                command += ch;
            }
            else if (iac && (ch == TN_IAC) && (!insb))
            {
                //2. seq. of two IACs
                iac = false;
                cleandata += ch;
                command = "";
            }
            else if( iac && ( ! insb) && ( (ch == TN_WILL) || (ch == TN_WONT) || (ch == TN_DO) || (ch == TN_DONT)))
            {
                //3. IAC DO/DONT/WILL/WONT
                iac = false;
                iac2 = true;
                command += ch;
            }
            else if( iac2 )
            {
                //4. IAC DO/DONT/WILL/WONT <command code>
                iac2 = false;
                command += ch;
                processTelnetCommand( command );
                command = "";
            }
            else if( iac && (!insb) && (ch == TN_SB))
            {
                //cout << getCurrentTime()<<" GOT TN_SB"<<endl;
                //5. IAC SB
                iac = false;
                insb = true;
                command += ch;
            }
            else if(iac && (!insb) && (ch == TN_SE))
            {
                //6. IAC SE without IAC SB - error - ignored
                command = "";
                iac = false;
            }
            else if( insb )
            {
                //7. inside IAC SB
                command += ch;
                if( iac && (ch == TN_SE))  //IAC SE - end of subcommand
                {
                    processTelnetCommand( command );
                    command = "";
                    iac = false;
                    insb = false;
                }
                if( iac ) iac = false;
                else if( ch == TN_IAC ) iac = true;
            }
            else
            //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
            {
                iac = false;
                command += ch;
                processTelnetCommand( command );
                //this could have set receivedGA to true; we'll handle that later
                command = "";
            }
        }
        else
        {
            if( ch != '\r' ) cleandata += ch;
        }

        if( recvdGA )
        {
            mGA_Driver = true;
            if( mCommands > 0 )
            {
                mCommands--;
                if( networkLatencyTime.elapsed() > 2000 )
                {
                    mCommands = 0;
                }
            }

            if( mUSE_IRE_DRIVER_BUGFIX || mLF_ON_GA )
            {
                cleandata.push_back('\n');//part of the broken IRE-driver bugfix to make up for broken \n-prepending in unsolicited lines, part #2 see line 628
            }
            recvdGA = false;
            gotPrompt( cleandata );
            cleandata = "";
        }
    }//for

    if( cleandata.size() > 0 )
    {
       gotRest( cleandata );
    }

    mpHost->mpConsole->finalize();
    if( mIsReplaying ) _loadReplay();
}

void cTelnet::handle_socket_signal_readyRead()
{
    mpHost->mInsertedMissingLF = false;

    if( mWaitingForResponse )
    {
        double time = networkLatencyTime.elapsed();
        networkLatency = time/1000;
        mWaitingForResponse = false;
    }

    char in_bufferx[100010];
    char* in_buffer = in_bufferx;
    char out_buffer[100010];

    int amount = socket.read( in_buffer, 100000 );
    in_buffer[amount+1] = '\0';
    if( amount == -1 ) return;
    if( amount == 0 ) return;

    string cleandata = "";
    int datalen;
    do {
    datalen = amount;
    char * buffer = in_buffer;
    if( mNeedDecompression )
    {
        datalen = decompressBuffer( in_buffer, amount, out_buffer );
        buffer = out_buffer;
    }
    buffer[datalen] = '\0';
    #ifdef DEBUG
        //qDebug()<<"got<"<<pBuffer<<">";
    #endif
    if( mpHost->mpConsole->mRecordReplay )
    {
        mpHost->mpConsole->mReplayStream << timeOffset.elapsed()-lastTimeOffset;
        mpHost->mpConsole->mReplayStream << datalen;
        mpHost->mpConsole->mReplayStream.writeRawData( &buffer[0], datalen );
    }

    recvdGA = false;
    for( int i = 0; i < datalen; i++ )
    {
        char ch = buffer[i];

        if( iac || iac2 || insb || (ch == TN_IAC) )
        {
            #ifdef DEBUG
                //qDebug() <<" SERVER SENDS telnet command "<<(unsigned int)ch;
            #endif
            if( ! (iac || iac2 || insb) && ( ch == TN_IAC ) )
            {
                iac = true;
                command += ch;
            }
            else if( iac && (ch == TN_IAC) && (!insb) )
            {
                //2. seq. of two IACs
                iac = false;
                cleandata += ch;
                command = "";
            }
            else if( iac && (!insb) && ((ch == TN_WILL) || (ch == TN_WONT) || (ch == TN_DO) || (ch == TN_DONT)))
            {
                //3. IAC DO/DONT/WILL/WONT
                iac = false;
                iac2 = true;
                command += ch;
            }
            else if(iac2)
            {
                //4. IAC DO/DONT/WILL/WONT <command code>
                iac2 = false;
                command += ch;
                processTelnetCommand( command );
                command = "";
            }
            else if( iac && (!insb) && (ch == TN_SB) )
            {
                //5. IAC SB
                iac = false;
                insb = true;
                command += ch;
            }
            else if( iac && (!insb) && (ch == TN_SE) )
            {
                //6. IAC SE without IAC SB - error - ignored
                command = "";
                iac = false;
            }
            else if( insb )
            {
                /*if( buffer[i] == static_cast<char>(200) )
                {
                    cout << "got atcp? ";
                    if( i > 1 )
                    {
                        if( ( buffer[i-2] == TN_IAC ) && ( buffer[i-1] == TN_SB ) )
                        {
                            atcp_msg = true;
                            cout << " yes"<<endl;
                        }
                        else
                            cout << "no"<<endl;
                    }
                }
                else*/
                if( ! mNeedDecompression )
                {
                    // IAC SB COMPRESS WILL SE for MCCP v1 (unterminated invalid telnet sequence)
                    // IAC SB COMPRESS2 IAC SE for MCCP v2
                    if( ( mMCCP_version_1 || mMCCP_version_2 ) && ( ! mNeedDecompression ) )
                    {
                        char _ch = buffer[i];
                        if( (_ch == OPT_COMPRESS ) || (_ch == OPT_COMPRESS2 ) )
                        {
                            bool _compress = false;
                            if( ( i > 1 ) && ( i+2 < datalen ) )
                            {
                                qDebug() << "checking mccp start seq...";
                                if( ( buffer[i-2] == TN_IAC ) && ( buffer[i-1] == TN_SB ) && ( buffer[i+1] == TN_WILL ) && ( buffer[i+2] == TN_SE ) )
                                {
                                    qDebug() << "MCCP version 1 starting sequence";
                                    _compress = true;
                                }
                                if( ( buffer[i-2] == TN_IAC ) && ( buffer[i-1] == TN_SB ) && ( buffer[i+1] == TN_IAC ) && ( buffer[i+2] == TN_SE ) )
                                {
                                    qDebug() << "MCCP version 2 starting sequence";
                                    _compress = true;
                                }
                                qDebug() << (int)buffer[i-2]<<","<<(int)buffer[i-1]<<","<<(int)buffer[i]<<","<<(int)buffer[i+1]<<","<<(int)buffer[i+2];
                            }
                            if( _compress )
                            {
                                mNeedDecompression = true;
                                // from this position in stream onwards, data will be compressed by zlib
                                gotRest( cleandata );
                                //mpHost->mpConsole->print("\n<starting MCCP data compression>\n");
                                cleandata = "";
                                initStreamDecompressor();
                                buffer += i + 3;//bugfix: BenH
                                int restLength = datalen - i - 3;
                                if( restLength > 0 )
                                {
                                    datalen = decompressBuffer( buffer, restLength, out_buffer );
                                    buffer = out_buffer;
                                    i = -1; // start processing buffer from the beginning.
                                } else {
                                    datalen = 0;
                                    i = -1; // end the loop, this will make i and datalen the same.
                                }
                                //bugfix: BenH
                                iac = false;
                                insb = false;
                                command = "";
                                ////////////////
                                goto MAIN_LOOP_END;
                            }
                        }
                    }
                }
                //7. inside IAC SB

                command += ch;
                if( iac && (ch == TN_SE) )  //IAC SE - end of subcommand
                {
                    processTelnetCommand( command );
                    command = "";
                    iac = false;
                    insb = false;
                }
                if(iac) iac = false;
                else if( ch == TN_IAC ) iac = true;
            }
            else
            //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
            {
                iac = false;
                command += ch;
                processTelnetCommand( command );
                //this could have set receivedGA to true; we'll handle that later
                command = "";
            }
        }
        else
        {
            if( ch != '\r' && ch != 0 ) cleandata += ch;
        }
MAIN_LOOP_END: ;
        if( recvdGA )
        {
            if( ! mFORCE_GA_OFF ) //FIXME: wird noch nicht richtig initialisiert
            {
                mGA_Driver = true;
                if( mCommands > 0 )
                {
                    mCommands--;
                    if( networkLatencyTime.elapsed() > 2000 )
                    {
                        mCommands = 0;
                    }
                }
                cleandata.push_back('\xff');
                recvdGA = false;
                gotPrompt( cleandata );
                cleandata = "";
            }
            else
            {
                if( mLF_ON_GA ) //TODO: reenable option in preferences
                {
                    cleandata.push_back('\n');
                }
            }
        }
    }//for
    } while (datalen == 100000);

    if( cleandata.size() > 0 )
    {
       gotRest( cleandata );
    }
    mpHost->mpConsole->finalize();
    lastTimeOffset = timeOffset.elapsed();
}
