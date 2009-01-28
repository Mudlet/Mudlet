/***************************************************************************
 *   copyright (C) 2002-2005 by Tomas Mecir (kmuddy@kmuddy.com)
 *   copyright (C) 2008 by Heiko Koehn (KoehnHeiko@googlemail.com
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
#include <QTcpSocket>
#include "mudlet.h"

#ifdef DEBUG
#undef DEBUG
#endif

//#define DEBUG


using namespace std;



cTelnet::cTelnet( Host * pH ) 
: mpHost(pH)
, mpPostingTimer( new QTimer( this ) )
{
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
    QString err = "TCP/IP socket ERROR:" + socket.errorString();
    postMessage( err );
}

void cTelnet::slot_send_login()
{
    qDebug()<<"sending login";
    sendData( mpHost->getLogin() );    
}

void cTelnet::slot_send_pass()
{
    qDebug()<<"sending pass";
    sendData( mpHost->getPass() );
}

void cTelnet::handle_socket_signal_connected()
{
    reset();
    QString msg = "A connection has been established successfully.\n";
    postMessage( msg );
    
    if( (mpHost->getLogin().size()>0) && (mpHost->getPass().size()>0) )
    {
        mTimerLogin->start(2000);
        mTimerPass->start(3000);
    }
    
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
    setDisplayDimensions();
}

void cTelnet::handle_socket_signal_disconnected()
{
    mNeedDecompression = false;
    reset();
    qDebug()<<"cTelnet::handle_socket_signal_disconnected() SOCKET LOST CONNECTION!";
    QString err = "\nSocket got disconnected. " + socket.errorString();
    postMessage( err );
}

void cTelnet::handle_socket_signal_hostFound(QHostInfo hostInfo)
{
				if(!hostInfo.addresses().isEmpty()) 
				{
        mHostAddress = hostInfo.addresses().first();
        QString msg	= "\nThe IP address of "+hostName+" has been found. It is: "+mHostAddress.toString()+"\n";
        postMessage( msg );
 	     	msg = "trying to connect to "+mHostAddress.toString()+":"+QString::number(hostPort)+" ...\n";
        postMessage( msg );
        socket.connectToHost(mHostAddress, hostPort);
    }
    else
    {
    				QString msg = "\nHost name lookup Failure! Connection cannot be established. The server name is not correct, not working properly, or your nameservers are not working properly.";
        postMessage( msg );
        return;
    }
}

bool cTelnet::sendData( const QString & data )
{
  string outdata = (outgoingDataCodec->fromUnicode(data)).data();
  outdata+="\r\n";
  return socketOutRaw(outdata);
}


bool cTelnet::socketOutRaw(string & data)
{
    if( ! socket.isWritable() )
    {
        qDebug()<<"MUDLET SOCKET ERROR: socket not connected, but wants to send data="<<data.c_str();
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
            qDebug()<<"MUDLET SOCKET ERROR: cant write all data to socket. sleeping(1)";
    		  		  // buffer full - try again
      		  		// FIXME: socket errors need to be checked here!
            continue;
        }
        remlen -= written;
        dataLength += written;
    } 
    while(remlen > 0);

    return true;
}



void cTelnet::setDisplayDimensions()
{
    //qDebug()<<"TELNET: enter NAWS setDisplayDimensions()";
  int x = 80;
  int y = 25;
  if(myOptionState[OPT_NAWS])
  {
      //      qDebug()<<"TELNET: sending NAWS 80x25";
    string s;
    s = (char) TN_IAC;
    s += (char) TN_SB;
    s += (char) OPT_NAWS;
    char x1, x2, y1, y2;
    x1 = (char) x / 256;
    x2 = (char) x % 256;
    y1 = (char) y / 256;
    y2 = (char) y % 256;
    //IAC must be doubled
    s += (char) x1;
    if (x1 == (char)TN_IAC)
      s += (char) TN_IAC;
    s += (char) x2; 
    if (x2 == (char)TN_IAC)
      s += (char) TN_IAC;
    s += (char) y1;
    if (y1 == (char)TN_IAC)
      s += (char) TN_IAC;
    s += (char) y2;
    if (y2 == (char)TN_IAC)
      s += (char) TN_IAC;
    
    s += (char) TN_IAC;
    s += (char) TN_SE;
    socketOutRaw(s);
  }
}

void cTelnet::sendTelnetOption (unsigned char type, unsigned char option)
{
    //cout << "CLIENT SENDING Telnet option: command<"<<(int)type<<"> option<"<<(int)option<<">"<<endl;		
    string cmd;
    cmd += (unsigned char) TN_IAC;
    cmd += type;
    cmd += option;
    socketOutRaw(cmd);
}



void cTelnet::processTelnetCommand (const string &command)
{
  string ayt_response = "delay\r\n";
  unsigned char ch = command[1];
  unsigned char option;
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
                       ( option == OPT_NAWS ) )
                   {
                       sendTelnetOption( TN_DO, option );
                       hisOptionState[option] = true;
                   }
                   else if( ( option == OPT_COMPRESS ) || ( option == OPT_COMPRESS2 ) )
                   {
                       //these are handled separately, as they're a bit special
                       if( ( option == OPT_COMPRESS ) && ( hisOptionState[OPT_COMPRESS2] ) )
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
                          cmd += (char) TN_IAC;
                          cmd += (char) TN_SB;
                          cmd += (char) OPT_STATUS;
                          cmd += (char) TNSB_IS;
                          for (short i = 0; i < 256; i++)
                          {
                              if (myOptionState[i])
                              {
                                  cmd += (char) TN_WILL;
                                  cmd += (char) i;
                              }
                              if (hisOptionState[i])
                              {
                                  cmd += (char) TN_DO;
                                  cmd += (char) i;
                              }
                          }
                          cmd += (char) TN_IAC;
                          cmd += (char) TN_SE;
                          socketOutRaw(cmd);
                      }
                  }
                  break;
          
              case OPT_TERMINAL_TYPE:
   
                  cout << "server sends telnet option terminal type"<<endl;
                  if( myOptionState[OPT_TERMINAL_TYPE] )
                  {
                      if(command[3] == TNSB_SEND )
                      {
                           //server wants us to send terminal type; he can send his own type
                           //too, but we just ignore it, as we have no use for it...
                           string cmd;
                           cmd += (char) TN_IAC;
                           cmd += (char) TN_SB;
                           cmd += (char) OPT_TERMINAL_TYPE;
                           cmd += (char) TNSB_IS;
                           cmd += termType.toLatin1().data();
                           cmd += (char) TN_IAC;
                           cmd += (char) TN_SE;
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
    mudlet::self()->printMessageOnDisplay( mpHost, msg );
}

//forward data for further processing

void cTelnet::gotLine( string & mud_data )
{
    return;
    //QString cd = incomingDataDecoder->toUnicode( mud_data.data(), mud_data.size() );
    /*qDebug()<<"\n------- recieved line <"<<cd<<"> -------";
    for( int i=0; i<cd.size(); i++ )
    {
        qDebug()<<"i="<<i<<" unicode="<<cd[i].unicode()<<" print="<<cd[i];
    } */
    //mpHost->gotLine( cd ); 
}

void cTelnet::gotPrompt( string & mud_data )
{
    //mMudData.append("\n");
    /*if( ! mIsTimerPosting )
    {
        mpPostingTimer->start();
        mIsTimerPosting = true;
    }*/
    //qDebug()<<"GA posting";
    mpPostingTimer->stop();
    mMudData += mud_data;
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
    if( mud_data[mud_data.size()-1] == '\n' )
    {
        //qDebug()<<"LF-posting";
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
    //qDebug()<<"TIMER-fired";
    if( ! mIsTimerPosting ) return;
    qDebug()<<"TIMER->posting";
    postData();
    mMudData = "";
    mIsTimerPosting = false; 
}

void cTelnet::postData()
{
    //cout << mMudData << endl;
    //qDebug()<<"postData packet size="<<mMudData.size();
    QString cd = incomingDataDecoder->toUnicode( mMudData.data(), mMudData.size() );
    //qDebug()<<"\n------- posting rest <"<<cd<<">-------";
    /*for( int i=0; i<cd.size(); i++ )
    {
        qDebug()<<"i="<<i<<" unicode="<<cd[i].unicode()<<" print="<<cd[i];
        int unicode = cd[i].unicode();
        if( (unicode < 32) || (unicode == 127) )
        {
            if( unicode == 27 ) continue;   // unicode 27 = escape
            //if( unicode == 13 ) continue;   // unicode 10 = line feed
            if( unicode == 10 ) continue;
            qDebug() << "deleting control character unicode="<<unicode;
            cd.remove( i, 1 );
        } 
    } */
    mpHost->gotRest( cd ); 
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
    char cleanBuffer[327690]; //clean data after decompression
    
    mZstream.avail_in = length;
    mZstream.next_in = (Bytef *) dirtyBuffer;
    
    mZstream.avail_out = 327680;
    mZstream.next_out = (Bytef *) cleanBuffer;
    
    int zval = inflate( & mZstream, Z_SYNC_FLUSH );
    int outSize = 327680 - mZstream.avail_out;
    
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

void cTelnet::handle_socket_signal_readyRead()
{
    char buffer[327690]; 
    int amount = socket.read( buffer, 327680 );
    buffer[amount+1] = '\0';
    //cout << "RAW_BUFFER_BEGIN<"<<buffer<<">RAW_BUFFER_END"<<endl;
    if( amount == -1 ) return; 
    if( amount == 0 ) return; 
  
    int datalen = amount;
    char * pBuffer = buffer;
    if( mNeedDecompression )
    {
        datalen = decompressBuffer( pBuffer, amount );
    }
    buffer[datalen] = '\0';
    string cleandata = "";
    recvdGA = false;
    for( unsigned int i = 0; i < (unsigned int) datalen; i++ )
    {
        unsigned char ch = buffer[i];
                  
        if( iac || iac2 || insb || (ch == (unsigned char)TN_IAC) )
        {
            unsigned char _ch = ch;
            //cout <<" SERVER SENDS telnet command "<<(int)_ch<<endl;
            if (! (iac || iac2 || insb) && ( ch == (unsigned char)TN_IAC ) )
            {
                iac = true;
                command += ch;
            }
            else if (iac && (ch == (unsigned char)TN_IAC) && (!insb)) 
            {
          		  		//2. seq. of two IACs
                iac = false;
                cleandata += ch;
                command = "";
            }
            else if(iac && (!insb) && ((ch == (unsigned char)TN_WILL) || (ch == (unsigned char)TN_WONT) || (ch == (unsigned char)TN_DO) || (ch == (unsigned char)TN_DONT)))
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
            else if(iac && (!insb) && (ch == (unsigned char)TN_SB))
            {
                //cout << getCurrentTime()<<" GOT TN_SB"<<endl;
                //5. IAC SB
                iac = false;
                insb = true;
                command += ch;
            }
            else if(iac && (!insb) && (ch == (unsigned char)TN_SE))
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
                    if( i+1 < (unsigned int) datalen )
                    {
                        unsigned char _ch = buffer[i];   
                        if( (_ch == (unsigned char)OPT_COMPRESS ) || (_ch == (unsigned char)OPT_COMPRESS2 ) )
                        {
                            cout << "COMPRESSION START sequence found"<<endl;
                            for( int _i = i; _i<datalen; _i++ )
                            {
                                mWaitingForCompressedStreamToStart = true;
                                cout << "looking for end of compression start sequence"<<endl;
                                _ch = buffer[_i];
                                if( _ch == (unsigned char)TN_SE )
                                {
                                    // start decompression MCCP version 1
                                    mNeedDecompression = true;
                                    setDisplayDimensions();
                                    // from this position in stream onwards, data will be compressed by zlib
                                    cout << "starting ZLIB decompression for MCCP version 1" << endl;
                                    gotRest( cleandata );
                                    cleandata = "";
                                    initStreamDecompressor();
                                    for( unsigned int _i2=0; _i2<i+_i+1;_i2++ ) pBuffer++;
                                    mWaitingForCompressedStreamToStart = false;
                                    int restLength = datalen-_i-1;
                                    if( restLength > 0 ) datalen = decompressBuffer( pBuffer, restLength );
                                    cout << "decompressed buffer len = "<<datalen<<endl;
                                    i = 0;
                                    goto MAIN_LOOP_END;
                                }
                            }
                        }
                    }
                }
                //7. inside IAC SB
                command += ch;
                if(iac && (ch == (unsigned char)TN_SE))  //IAC SE - end of subcommand
                {
                    // start decompression MCCP version 2
                    if( mWaitingForCompressedStreamToStart )
                    {
                        mNeedDecompression = true;
                        // from this position in stream onwards, data will be compressed by zlib
                        cout << "starting ZLIB decompression for MCCP version 2" << endl;
                        gotRest( cleandata );
                        cleandata = "";
                        initStreamDecompressor();
                        for( unsigned int _i=0; _i<i+1;_i++ ) pBuffer++;
                        mWaitingForCompressedStreamToStart = false;
                        int restLength = datalen-i-1;
                        if( restLength > 0 ) datalen = decompressBuffer( pBuffer, restLength );
                        i = 0;
                        goto MAIN_LOOP_END;
                    }
                    processTelnetCommand( command );
                    command = "";
                    iac = false;
                    insb = false;
                }
                if(iac) iac = false;
                else if( ch == (unsigned char)TN_IAC ) iac = true;
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
            if( ch != '\r' ) 
                cleandata += ch;
        }
        MAIN_LOOP_END: ;
    }//for
    if(recvdGA)
    {
        //cout << getCurrentTime() << " GOT telnet command TN_GA" <<endl;
        //we got a prompt
        //cleandata.push_back('\n');
        gotPrompt( cleandata );
        cleandata = "";
        recvdGA = false;
    }
    if( cleandata.size() > 0 ) 
    {
        gotRest( cleandata );
    }
}



