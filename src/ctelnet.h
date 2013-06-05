/***************************************************************************

copyright (C) 2002-2005 by Tomas Mecir (kmuddy@kmuddy.com)
copyright (c) 2008-2009 by Heiko Koehn (koehnheiko@googlemail.com)

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
//#include <sys/time.h>
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
#include <QColor>
#include "dlgComposer.h"
#include <QNetworkAccessManager>
#include <QProgressDialog>

const char TN_SE = 240;
const char TN_NOP = 241;
const char TN_DM = 242;
const char TN_B = 243;
const char TN_IP = 244;
const char TN_AO = 245;
const char TN_AYT = 246;
const char TN_EC = 247;
const char TN_EL = 248;
const char TN_GA = 249;
const char TN_SB = 250;
const char TN_WILL = 251;
const char TN_WONT = 252;
const char TN_DO = 253;
const char TN_DONT = 254;
const char TN_IAC = 255;
const char TN_EOR = 239;

const char GMCP = 201; /* GMCP */
const char MXP = 91; //MXP

const char OPT_ECHO = 1;
const char OPT_SUPPRESS_GA = 3;
const char OPT_STATUS = 5;
const char OPT_TIMING_MARK = 6;
const char OPT_TERMINAL_TYPE = 24;
const char OPT_EOR = 25;
const char OPT_NAWS = 31;
const char OPT_COMPRESS = 85;
const char OPT_COMPRESS2 = 86;
const char OPT_MSP = 90;
const char OPT_MXP = 91;
const char TNSB_IS = 0;
const char TNSB_SEND = 1;




class mudlet;
class Host;
//class dlgComposer;

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
  void                setATCPVariables( QString & _msg );
  void                setGMCPVariables( QString & _msg );
  void                atcpComposerCancel();
  void                atcpComposerSave( QString );
  void                setLPMudStyle ( bool lpmustyle );
  void                setNegotiateOnStartup( bool startupneg );
  void                setDisplayDimensions();
  void                encodingChanged(QString encoding);
  void                set_USE_IRE_DRIVER_BUGFIX( bool b ){ mUSE_IRE_DRIVER_BUGFIX=b; }
  void                set_LF_ON_GA( bool b ){ mLF_ON_GA=b; }
  void                recordReplay();
  void                loadReplay( QString & );
  void                _loadReplay();

  void                setChannel102Variables( QString & );




  bool                socketOutRaw(std::string & data);

  QMap<int,bool>      supportedTelnetOptions;
  bool                mResponseProcessed;
  double              networkLatency;
  QTime               networkLatencyTime;
  bool                mAlertOnNewData;
  bool                mGA_Driver;
  bool                mFORCE_GA_OFF;
  dlgComposer *       mpComposer;
  QNetworkAccessManager * mpDownloader;
  QProgressDialog *   mpProgressDialog;
  QString             mServerPackage;

public slots:
  void                setDownloadProgress( qint64, qint64 );
  void                replyFinished( QNetworkReply * );
  void                readPipe();
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

  void                processTelnetCommand (const std::string &command);
  void                sendTelnetOption( char type, char option);
  //string getCurrentTime(); //NOTE: not w32 compatible
  void                gotRest( std::string & );
  void                gotPrompt( std::string & );
  void                postData();



  Host *              mpHost;
  QTcpSocket          socket;
  QHostAddress        mHostAddress;
  QTextCodec *        incomingDataCodec;
  QTextCodec *        outgoingDataCodec;
  QTextDecoder *      incomingDataDecoder;
  QTextEncoder *      outgoingDataDecoder;
  QString             hostName;
  int                 hostPort;
  QDataStream         mOfs;
  double              networkLatencyMin;
  double              networkLatencyMax;
  bool                mWaitingForResponse;
  std::queue<int>     mCommandQueue;

  z_stream            mZstream;

  bool                mNeedDecompression;
  bool                mWaitingForCompressedStreamToStart;
  std::string         command;
  bool                iac, iac2, insb;
  bool                myOptionState[256], hisOptionState[256];
  bool                announcedState[256];
  bool                heAnnouncedState[256];
  bool                triedToEnable[256];
  bool                recvdGA;

  int                 curX, curY;
  QString             termType;
  QString             encoding;
  QTimer *            mpPostingTimer;
  bool                mUSE_IRE_DRIVER_BUGFIX;
  bool                mLF_ON_GA;

  int                 mCommands;
  bool                mMCCP_version_1;
  bool                mMCCP_version_2;


  std::string         mMudData;
  bool                mIsTimerPosting;
  QTimer *            mTimerLogin;
  QTimer *            mTimerPass;
  QTime               timeOffset;
  QTime               mConnectionTime;
  int                 lastTimeOffset;
  bool                enableATCP;
  bool                enableGMCP;
  bool                enableChannel102;


};

#endif


