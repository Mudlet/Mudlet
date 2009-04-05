/***************************************************************************
    
copyright (C) 2002-2005 by Tomas Mecir (kmuddy@kmuddy.com)
copyright (c) 2008 by Heiko Koehn (koehnheiko@googlemail.com)
 
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef CTELNET_H
#define CTELNET_H

#include <QObject>
#include <list>
#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <queue>
#include <QQueue>
#include <QTextCodec>
#include <QHostAddress>
#include <QTcpSocket>
#include <QHostInfo>
#include <zlib.h>
#include <QTimer>
#include <QTime>

#define TN_SE 240
#define TN_NOP 241
#define TN_DM 242
#define TN_B 243
#define TN_IP 244
#define TN_AO 245
#define TN_AYT 246
#define TN_EC 247
#define TN_EL 248
#define TN_GA 249
#define TN_SB 250
#define TN_WILL 251
#define TN_WONT 252
#define TN_DO 253
#define TN_DONT 254
#define TN_IAC 255
#define OPT_ECHO 1
#define OPT_SUPPRESS_GA 3
#define OPT_STATUS 5
#define OPT_TIMING_MARK 6
#define OPT_TERMINAL_TYPE 24
#define OPT_NAWS 31
#define OPT_COMPRESS 85
#define OPT_COMPRESS2 86
#define OPT_MSP 90
#define OPT_MXP 91
#define TNSB_IS 0
#define TNSB_SEND 1


#include <QColor>

class mudlet;
class Host;


class cTelnet : public QObject
{
Q_OBJECT

public: 
                      cTelnet( Host * pH );
                     ~cTelnet();
  void                connectIt(const QString &address, int port);
  void                disconnect();
  bool                sendData ( QString & data );
  void                setCommandEcho( bool cmdEcho );
  void                setLPMudStyle ( bool lpmustyle );
  void                setNegotiateOnStartup( bool startupneg );
  void                setDisplayDimensions();
  void                encodingChanged(QString encoding);
  void                set_USE_IRE_DRIVER_BUGFIX( bool b ){ mUSE_IRE_DRIVER_BUGFIX=b; }

  bool                mResponseProcessed;
  QTime               networkLatencyTime;
  double              networkLatency;

protected slots:
  
  void                handle_socket_signal_hostFound(QHostInfo);
  void                handle_socket_signal_connected();
  void                handle_socket_signal_disconnected();
  void                handle_socket_signal_readyRead();
  void                handle_socket_signal_error();
  void                slot_timerPosting();
  void                slot_send_login();
  void                slot_send_pass();

private:
                      cTelnet(){;}    
  void                initStreamDecompressor();  
  int                 decompressBuffer( char * dirtyBuffer, int length );
  void                postMessage( QString msg );  
  void                reset();
  void                connectionFailed();
  bool                socketOutRaw(std::string & data);
  void                processTelnetCommand (const std::string &command);
  void                sendTelnetOption(unsigned char type, unsigned char option);
  //string getCurrentTime(); //NOTE: not w32 compatible
  void                gotRest( std::string & );
  void                gotLine( std::string & );
  void                gotPrompt( std::string & );
  void                postData();  
    
  bool                mUSE_IRE_DRIVER_BUGFIX;
  Host *              mpHost;  
  QTcpSocket          socket;
  QHostAddress        mHostAddress;
  QTextCodec *        incomingDataCodec;
  QTextCodec *        outgoingDataCodec;
  QTextDecoder *      incomingDataDecoder;
  QTextEncoder *      outgoingDataDecoder;
  QString             hostName;
  int                 hostPort;

  double              networkLatencyMin;
  double              networkLatencyMax;
  bool                mWaitingForResponse;
  std::queue<int>     mCommandQueue;
  int                 mCommands;
  z_stream            mZstream;  
  bool                mMCCP_version_1;
  bool                mMCCP_version_2;
  bool                mNeedDecompression; 
  bool                mWaitingForCompressedStreamToStart;
  std::string         command;
  bool                iac, iac2, insb;
  bool                myOptionState[256], hisOptionState[256];
  bool                announcedState[256];
  bool                heAnnouncedState[256];
  bool                triedToEnable[256];
  bool                recvdGA;
  bool                mGA_Driver;
  int                 curX, curY;
  QString             termType;
  QString             encoding;
  QTimer *            mpPostingTimer;
  std::string         mMudData;
  bool                mIsTimerPosting;
  QTimer *            mTimerLogin;
  QTimer *            mTimerPass;
};

#endif


