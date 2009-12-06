/***************************************************************************
 *   copyright (C) 2002-2005 by Tomas Mecir (kmuddy@kmuddy.com)
 *   copyright (C) 2008-2009 by Heiko Koehn (KoehnHeiko@googlemail.com
 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ctelnet.h"
#include <time.h>
#include <unistd.h>
#include <QTextCodec>
#include <QHostAddress>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <stdio.h>
#include <QDebug>
#include <QDir>
#include <QTcpSocket>
#include "mudlet.h"
#include "TDebug.h"

#ifdef DEBUG
#undef DEBUG
#endif

//#define DEBUG


using namespace std;



cTelnet::cTelnet( Host * pH ) 
: mpHost(pH)
, mpPostingTimer( new QTimer( this ) )
, mUSE_IRE_DRIVER_BUGFIX( false )
, mLF_ON_GA( false )
, mResponseProcessed( true )
, mGA_Driver( false )
, mCommands( 0 )
{
    if( mpHost )
    {
        //qDebug()<<"hostName="<<mpHost->getName()<<" bugfix="<<mpHost->mUSE_IRE_DRIVER_BUGFIX;
        mUSE_IRE_DRIVER_BUGFIX = mpHost->mUSE_IRE_DRIVER_BUGFIX;
        mLF_ON_GA = mpHost->mLF_ON_GA;
    }
    mIsTimerPosting = false;
    mNeedDecompression = false;
    mWaitingForCompressedStreamToStart = false;
    // initialize default encoding
    encoding = "UTF-8";
    encodingChanged(encoding);
    termType = "Mudlet 1.0";
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
    reset ();
        
    mpPostingTimer->setInterval( 100 ); // 10 times per second
    connect(mpPostingTimer, SIGNAL(timeout()), this, SLOT(slot_timerPosting()));
    
    mTimerLogin = new QTimer( this );
    mTimerLogin->setSingleShot(true);
    connect(mTimerLogin, SIGNAL(timeout()), this, SLOT(slot_send_login()));
    
    mTimerPass = new QTimer( this );
    mTimerPass->setSingleShot( true );
    connect(mTimerPass, SIGNAL(timeout()), this, SLOT(slot_send_pass()));
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
}


cTelnet::~cTelnet()
{
    disconnect();
    socket.deleteLater();
}


void cTelnet::encodingChanged(QString encoding)
{
    //cout << "cTelnet::encodingChanged() called!" << endl;
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
    if( socket.state() != QAbstractSocket::UnconnectedState ) return;
    hostName = address;
    hostPort = port;
 
    QString server = "looking up the IP address of server:" + address + ":" + QString::number(port) + " ...\n";
    postMessage( server );
    QHostInfo::lookupHost(address, this, SLOT(handle_socket_signal_hostFound(QHostInfo)));
}


void cTelnet::disconnect ()
{
    socket.disconnectFromHost();
}

void cTelnet::handle_socket_signal_error()
{
    QString err = "TCP/IP socket ERROR:" + socket.errorString() + "\n";
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
    QString msg = "A connection has been established successfully.\n";
    postMessage( msg );
    mConnectionTime.start();
    if( (mpHost->getLogin().size()>0) && (mpHost->getPass().size()>0) )
    {
        mTimerLogin->start(2000);
        mTimerPass->start(3000);
    }
    /*
    //negotiate some telnet options, if allowed
    string cmd;
    //NAWS (used to send info about window size)
    cmd += TN_IAC;
    cmd += TN_WILL;
    cmd += OPT_NAWS;
    //do not allow server to echo our text!
    cmd += TN_IAC;
    cmd += TN_DONT;
    cmd += OPT_ECHO;
    socketOutRaw(cmd);
    setDisplayDimensions();*/
}

void cTelnet::handle_socket_signal_disconnected()
{
    postData();
    QString msg;
    QTime timeDiff(0,0,0,0);
    msg = QString("connection time: %1\n").arg(timeDiff.addMSecs(mConnectionTime.elapsed()).toString("hh:mm:ss.zzz"));
    mNeedDecompression = false;
    reset();
    QString lf = "\n\n";
    QString err = "Socket got disconnected. " + socket.errorString() + "\n";
    QString spacer = "-------------------------------------------------------------\n";
    if( ! mpHost->mIsGoingDown )
    {
        postMessage( lf );
        postMessage( spacer );
        postMessage( err );
        postMessage( msg );
        postMessage( spacer );
    }
}

void cTelnet::handle_socket_signal_hostFound(QHostInfo hostInfo)
{
    if(!hostInfo.addresses().isEmpty())
    {
        mHostAddress = hostInfo.addresses().first();
        QString msg = "The IP address of "+hostName+" has been found. It is: "+mHostAddress.toString()+"\n";
        postMessage( msg );
        msg = "trying to connect to "+mHostAddress.toString()+":"+QString::number(hostPort)+" ...\n";
        postMessage( msg );
        socket.connectToHost(mHostAddress, hostPort);
    }
    else
    {
        socket.connectToHost(hostInfo.hostName(), hostPort);
        QString msg = "Host name lookup Failure! Connection cannot be established. The server name is not correct, not working properly, or your nameservers are not working properly.\n";
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
    string outdata = (outgoingDataCodec->fromUnicode(data)).data();
    if( ! mpHost->mUSE_UNIX_EOL )
    {
        outdata.append("\r\n");
    }
    else
        outdata += "\n";
    //cout<<"OUT:<"<<outdata<<">"<<endl;
    return socketOutRaw( outdata );
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
    int x = mpHost->mScreenWidth;
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

void cTelnet::sendTelnetOption (char type, char option)
{
    //cout << "CLIENT SENDING Telnet option: command<"<<(int)type<<"> option<"<<(int)option<<">"<<endl;		
    string cmd;
    cmd += TN_IAC;
    cmd += type;
    cmd += option;
    socketOutRaw(cmd);
}



void cTelnet::processTelnetCommand (const string &command)
{
  string ayt_response = "delay\r\n";
  char ch = command[1];
  int option;
  switch(ch) 
  {
      case TN_AYT: 
          //FIXME
          cout << "WARNING: FIXME: cTelnet::processTelnetCommand() command = TN_AYT"<<endl;
          break;

      case TN_GA:
          #ifdef DEBUG
            cout << "cTelnet::processTelnetCommand() command = TN_GA"<<endl;
          #endif
          recvdGA = true;
          break;

      case TN_WILL:
          //server wants to enable some option (or he sends a timing-mark)...
          option = command[2];
          #ifdef DEBUG
            cout << "cTelnet::processTelnetCommand() command = TN_WILL option="<<(int)option<<endl;
          #endif
          heAnnouncedState[option] = true;
          if( triedToEnable[option] )
          {
              hisOptionState[option] = true;
              triedToEnable[option] = false;
          }
          else
          {
              if( !hisOptionState[option] )
              {
                   //only if this is not set; if it's set, something's wrong wth the server
                   //(according to telnet specification, option announcement may not be
                   //unless explicitly requested)

                   if( ( option == OPT_SUPPRESS_GA ) || 
                       ( option == OPT_STATUS ) ||
                       ( option == OPT_TERMINAL_TYPE) || 
                       ( option == OPT_ECHO ) ||
                       ( option == OPT_NAWS ) )
                   {
                       sendTelnetOption( TN_DO, option );
                       hisOptionState[option] = true;
                   }
                   else if( ( option == OPT_COMPRESS ) || ( option == OPT_COMPRESS2 ) )
                   {
                       //these are handled separately, as they're a bit special
                       if( ( option == OPT_COMPRESS ) && ( hisOptionState[static_cast<int>(OPT_COMPRESS2)] ) )
                       {
                           //protocol says: reject MCCP v1 if you have previously accepted
                           //MCCP v2...
                           sendTelnetOption( TN_DONT, option );
                           hisOptionState[option] = false;
                           cout << "Rejecting MCCP v1, because v2 has already been negotiated." << endl;
                       }
                       else
                       {
                           sendTelnetOption( TN_DO, option );
                           hisOptionState[option] = true;
                           //inform MCCP object about the change
                           if( ( option == OPT_COMPRESS ) ) 
                           {
                               mMCCP_version_1 = true;
                               //MCCP->setMCCP1(true);
                               cout << "MCCP v1 enabled." << endl;
                           }
                           else 
                           {
                               mMCCP_version_2 = true;
                               //MCCP->setMCCP2( true );
                               cout << "MCCP v2 enabled !" << endl;
                           }
                       }
                   }
                   else
                   {
                       sendTelnetOption( TN_DONT, option );
                       hisOptionState[option] = false;
                   }
            //}    
          }
          break;

      case TN_WONT:
          
          //server refuses to enable some option...
          #ifdef DEBUG
              cout << "cTelnet::processTelnetCommand() command = TN_WONT"<<endl;
          #endif
          option = command[2];
          if( triedToEnable[option] )
          {
              hisOptionState[option] = false;
              triedToEnable[option] = false;
              heAnnouncedState[option] = true;
          }
          else
          {
              //send DONT if needed (see RFC 854 for details)
              if( hisOptionState[option] || ( heAnnouncedState[option] ) )
              {
                  sendTelnetOption( TN_DONT, option );
                  hisOptionState[option] = false;
                  
                  if( ( option == OPT_COMPRESS ) ) 
                  {
                      //MCCP->setMCCP1 (false);
                      mMCCP_version_1 = false;
                      cout << "MCCP v1 disabled !" << endl;
                  }
                  if( ( option == OPT_COMPRESS2 ) ) 
                  {
                      mMCCP_version_2 = false;
                      //MCCP->setMCCP2 (false);
                      //      cout << "MCCP v1 disabled !" << endl;
                  }
              }
              heAnnouncedState[option] = true;
          }
          break;

      case TN_DO:
          
          //server wants us to enable some option
          option = command[2];
              //cout << "server wants us to enable telnet option " << (int)option << "(TN_DO + "<< (int)option<<")"<<endl;
          if(option == OPT_TIMING_MARK)
          {
              cout << "OK we are willing to enable TIMING_MARK" << endl;
              //send WILL TIMING_MARK
              sendTelnetOption( TN_WILL, option );
          }
          else if (!myOptionState[255])
          //only if the option is currently disabled
          {
              if( ( option == OPT_SUPPRESS_GA ) || 
                  ( option == OPT_STATUS ) || 
                  ( option == OPT_NAWS ) || 
                  ( option == OPT_TERMINAL_TYPE ) ) 
              {
                  if( option == OPT_SUPPRESS_GA ) cout << "OK we are willing to enable option SUPPRESS_GA"<<endl;
                  if( option == OPT_STATUS ) cout << "OK we are willing to enable telnet option STATUS"<<endl;
                  if( option == OPT_TERMINAL_TYPE ) cout << "OK we are willing to enable telnet option TERMINAL_TYPE"<<endl;
                  if( option == OPT_NAWS ) cout << "OK we are willing to enable telnet option NAWS"<<endl;
                  sendTelnetOption( TN_WILL, option );
                  myOptionState[option] = true;
                  announcedState[option] = true;
              }
              else
              {
                  cout << "SORRY, we are NOT WILLING to enable this telnet option." << endl;
                  sendTelnetOption (TN_WONT, option);
                  myOptionState[option] = false;
                  announcedState[option] = true;
              }
          }
          if( option == OPT_NAWS )
          {    
          				//NAWS 
              setDisplayDimensions();
          }
          break;

      case TN_DONT:
          
          //only respond if value changed or if this option has not been announced yet
              //cout << "cTelnet::processTelnetCommand() command = TN_DONT"<<endl;
          option = command[2];
          if( myOptionState[option] || ( !announcedState[option] ) )
          {
              sendTelnetOption (TN_WONT, option);
              announcedState[option] = true;
          }
          myOptionState[option] = false;
          break;

      case TN_SB:
          
          //subcommand - we analyze and respond...
              //cout << "cTelnet::processTelnetCommand() command = TN_SB"<<endl;
         
          option = command[2];
          
          switch (option) //switch 2
          {
              case OPT_STATUS:
                  
                  //see OPT_TERMINAL_TYPE for explanation why I'm doing this
                  if( true )
                  {
                      cout << "WARNING: FIXME #501" << endl;
                      if(command[3] == TNSB_SEND)
                      {
                          cout << "WARNING: FIXME #504" << endl;
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
          
              case OPT_TERMINAL_TYPE:
   
                  cout << "server sends telnet option terminal type"<<endl;
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
          };//end switch 2
          //other commands are simply ignored (NOP and such, see .h file for list)
      }
  };//end switch 1
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

void cTelnet::postMessage( QString msg )
{
#ifdef DEBUG 
    qDebug() << " POSTING to message to GUI: " << msg;
#endif
    mudlet::self()->printSystemMessage( mpHost, msg );
}

//forward data for further processing


void cTelnet::gotPrompt( string & mud_data )
{
    mpPostingTimer->stop();
    mMudData += mud_data;
    
    if( mUSE_IRE_DRIVER_BUGFIX )
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
    if( ( ! mGA_Driver ) || ( mud_data[mud_data.size()-1] == '\n' ) )
    {
        mpPostingTimer->stop();
        mMudData += mud_data;
        
        postData();
        mMudData = "";
        mIsTimerPosting = false;
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
    postData();
    mMudData = "";
    mIsTimerPosting = false; 
}

void cTelnet::postData()
{
    //QString cd = incomingDataDecoder->toUnicode( mMudData.data(), mMudData.size() );
    mpHost->mpConsole->printOnDisplay( mMudData );
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

int cTelnet::decompressBuffer( char * dirtyBuffer, int length )
{
    char cleanBuffer[100001]; //clean data after decompression
    
    mZstream.avail_in = length;
    mZstream.next_in = (Bytef *) dirtyBuffer;
    
    mZstream.avail_out = 100000;
    mZstream.next_out = (Bytef *) cleanBuffer;
    
    int zval = inflate( & mZstream, Z_SYNC_FLUSH );
    int outSize = 100000 - mZstream.avail_out;
    
    if (zval == Z_STREAM_END)
    {
        inflateEnd( & mZstream );
    }
    else
    {
        if (zval < 0)  
        {
            initStreamDecompressor();
            return -1;
        }
    }
    memcpy( dirtyBuffer, cleanBuffer, outSize );
    return outSize;
}


void cTelnet::recordReplay()
{
    lastTimeOffset = 0;
    timeOffset.start();
}

char loadBuffer[100001];
int loadedBytes;
QDataStream replayStream;
bool loadingReplay;
QFile replayFile;

void cTelnet::loadReplay( QString & name )
{
    replayFile.setFileName( name );
    QString msg = "loading replay " + name;
    postMessage( msg );
    replayFile.open( QIODevice::ReadOnly );
    replayStream.setDevice( &replayFile );
    loadingReplay = true;
    mudlet::self()->replayStart();
    _loadReplay();
}

void cTelnet::_loadReplay()
{
    if( ! replayStream.atEnd() )
    {
        int offset;
        int amount;
        replayStream >> offset;
        replayStream >> amount;

        char * pB = &loadBuffer[0];
        loadedBytes = replayStream.readRawData ( pB, amount );
        qDebug()<<"loaded:"<<loadedBytes<<"/"<<amount<<" bytes"<<" waiting for "<<offset<<" milliseconds";
        loadBuffer[loadedBytes+1] = '\0';
        QTimer::singleShot( offset/mudlet::self()->mReplaySpeed, this, SLOT(readPipe()));
        mudlet::self()->mReplayTime = mudlet::self()->mReplayTime.addMSecs(offset);
    }
    else
    {
        loadingReplay = false;
        replayFile.close();
        QString msg = "The replay has ended.\n";
        postMessage( msg );
        mudlet::self()->replayOver();
    }
}


void cTelnet::readPipe()
{
    int datalen = loadedBytes;
    string cleandata = "";
    recvdGA = false;
    for( int i = 0; i < datalen; i++ )
    {
        char ch = loadBuffer[i];

        if( iac || iac2 || insb || (ch == TN_IAC) )
        {
            #ifdef DEBUG
                cout <<" SERVER SENDS telnet command "<<(int)_ch<<endl;
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
MAIN_LOOP_END: ;
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
    if( loadingReplay ) _loadReplay();
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

    char buffer[100010];
    bool gotData = false;

    int amount = socket.read( buffer, 100000 );
    buffer[amount+1] = '\0';
    if( amount == -1 ) return; 
    if( amount == 0 ) return; 
  
    int datalen = amount;
    char * pBuffer = buffer;
    if( mNeedDecompression )
    {
        datalen = decompressBuffer( pBuffer, amount );
    }
    buffer[datalen] = '\0';
    #ifdef DEBUG
        cout<<"got<";
    #endif
    if( mpHost->mpConsole->mRecordReplay )
    {
        mpHost->mpConsole->mReplayStream << timeOffset.elapsed()-lastTimeOffset;
        mpHost->mpConsole->mReplayStream << datalen;
        mpHost->mpConsole->mReplayStream.writeRawData( &buffer[0], datalen );
    }

    string cleandata = "";
    recvdGA = false;
    for( int i = 0; i < datalen; i++ )
    {
        char ch = buffer[i];
                  
        if( iac || iac2 || insb || (ch == TN_IAC) )
        {
            char _ch = ch;
            #ifdef DEBUG
                cout <<" SERVER SENDS telnet command "<<(int)_ch<<endl;
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
                //cout << getCurrentTime()<<" GOT TN_SB"<<endl;
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
                if( ! mNeedDecompression )
                {
                    cout << " looking for MCCP to initialize"<<endl;
                    // IAC SB COMPRESS WILL SE for MCCP v1 (unterminated invalid telnet sequence)
                    // IAC SB COMPRESS2 IAC SE for MCCP v2    
                    if( i+1 < datalen )
                    {
                        char _ch = buffer[i];
                        if( (_ch == OPT_COMPRESS ) || (_ch == OPT_COMPRESS2 ) )
                        {
                            cout << "COMPRESSION START sequence found"<<endl;
                            for( int _i = i; _i<datalen; _i++ )
                            {
                                mWaitingForCompressedStreamToStart = true;
                                cout << "looking for end of compression start sequence"<<endl;
                                _ch = buffer[_i];
                                if( _ch == TN_SE )
                                {
                                    // start decompression MCCP version 1
                                    mNeedDecompression = true;
                                    setDisplayDimensions();
                                    // from this position in stream onwards, data will be compressed by zlib
                                    cout << "starting ZLIB decompression. ";
                                    if( _ch == OPT_COMPRESS )
                                        cout << "MCCP version 1" << endl;
                                    else
                                        cout << "MCCP version 2" << endl;
                                    gotRest( cleandata );
                                    cleandata = "";
                                    initStreamDecompressor();
                                    pBuffer += _i+1;
                                    mWaitingForCompressedStreamToStart = false;
                                    int restLength = datalen-_i-1;
                                    if( restLength > 0 ) datalen = decompressBuffer( pBuffer, restLength );
                                    i = 0;
                                    goto MAIN_LOOP_END;
                                }
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
            if( ch != '\r' ) cleandata += ch;
        }
MAIN_LOOP_END: ;
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
    lastTimeOffset = timeOffset.elapsed();
}



