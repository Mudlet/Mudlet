#ifndef MUDLET_CTELNET_H
#define MUDLET_CTELNET_H

/***************************************************************************
 *   Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com            *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2015 by Florian Scheel - keneanung@googlemail.com  *
 *   Copyright (C) 2015, 2017 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "pre_guard.h"
#include <QHostAddress>
#include <QHostInfo>
#include <QPointer>
#include <QStringList>
#include <QTcpSocket>
#include <QTime>
#include "post_guard.h"

#include <zlib.h>

#include <iostream>
#include <queue>
#include <string>

class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QTextCodec;
class QTextDecoder;
class QTextEncoder;
class QTimer;

class Host;
class dlgComposer;


const char TN_SE = static_cast<char>(240);
const char TN_NOP = static_cast<char>(241);
const char TN_DM = static_cast<char>(242);
const char TN_B = static_cast<char>(243);
const char TN_IP = static_cast<char>(244);
const char TN_AO = static_cast<char>(245);
const char TN_AYT = static_cast<char>(246);
const char TN_EC = static_cast<char>(247);
const char TN_EL = static_cast<char>(248);
const char TN_GA = static_cast<char>(249);
const char TN_SB = static_cast<char>(250);
const char TN_WILL = static_cast<char>(251);
const char TN_WONT = static_cast<char>(252);
const char TN_DO = static_cast<char>(253);
const char TN_DONT = static_cast<char>(254);
const char TN_IAC = static_cast<char>(255);
const char TN_EOR = static_cast<char>(239);
const char TN_BELL = static_cast<char>(7);

const char GMCP = static_cast<char>(201);
const char MXP = 91;
const char MSDP = 69; // http://tintin.sourceforge.net/msdp/

const char OPT_ECHO = 1;
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

class cTelnet : public QObject
{
    Q_OBJECT

    Q_DISABLE_COPY(cTelnet)

public:
    cTelnet(Host* pH);
    ~cTelnet();
    void connectIt(const QString& address, int port);
    void disconnect();
    bool sendData(QString& data);
    void setATCPVariables(const QString& _msg);
    void setGMCPVariables(const QString& _msg);
    void atcpComposerCancel();
    void atcpComposerSave(QString);
    void setDisplayDimensions();
    void encodingChanged(const QString&);
    void set_USE_IRE_DRIVER_BUGFIX(bool b) { mUSE_IRE_DRIVER_BUGFIX = b; }
    void set_LF_ON_GA(bool b) { mLF_ON_GA = b; }
    void recordReplay();
    void loadReplay(QString&);
    void _loadReplay();
    bool isReplaying() { return loadingReplay; }
    void setChannel102Variables(const QString&);
    bool socketOutRaw(std::string& data);
    const QString & getEncoding() const { return mEncoding; }
    QPair<bool, QString> setEncoding(const QString &, const bool isToStore = true);
    void postMessage(QString);
    const QStringList & getEncodingsList() const { return mAcceptableEncodings; }

    QMap<int, bool> supportedTelnetOptions;
    bool mResponseProcessed;
    double networkLatency;
    QTime networkLatencyTime;
    bool mAlertOnNewData;
    bool mGA_Driver;
    bool mFORCE_GA_OFF;
    dlgComposer* mpComposer;
    QNetworkAccessManager* mpDownloader;
    QProgressDialog* mpProgressDialog;
    QString mServerPackage;

public slots:
    void setDownloadProgress(qint64, qint64);
    void replyFinished(QNetworkReply*);
    void readPipe();
    void handle_socket_signal_hostFound(QHostInfo);
    void handle_socket_signal_connected();
    void handle_socket_signal_disconnected();
    void handle_socket_signal_readyRead();
    void handle_socket_signal_error();
    void slot_timerPosting();
    void slot_send_login();
    void slot_send_pass();


private:
    cTelnet() {}
    void initStreamDecompressor();
    int decompressBuffer(char*& in_buffer, int& length, char* out_buffer);
    void reset();

    void processTelnetCommand(const std::string& command);
    void sendTelnetOption(char type, char option);
    void gotRest(std::string&);
    void gotPrompt(std::string&);
    void postData();
    void raiseProtocolEvent(const QString& name, const QString& protocol);

    QPointer<Host> mpHost;
    QTcpSocket socket;
    QHostAddress mHostAddress;
    QTextCodec* incomingDataCodec;
    QTextCodec* outgoingDataCodec;
    QTextDecoder* incomingDataDecoder;
    QTextEncoder* outgoingDataEncoder;
    QString hostName;
    int hostPort;
    double networkLatencyMin;
    double networkLatencyMax;
    bool mWaitingForResponse;
    std::queue<int> mCommandQueue;

    z_stream mZstream;

    bool mNeedDecompression;
    bool mWaitingForCompressedStreamToStart;
    std::string command;
    bool iac, iac2, insb;
    bool myOptionState[256], hisOptionState[256];
    bool announcedState[256];
    bool heAnnouncedState[256];
    bool triedToEnable[256];
    bool recvdGA;

    int curX, curY;
    QString termType;
    QString mEncoding;
    QTimer* mpPostingTimer;
    bool mUSE_IRE_DRIVER_BUGFIX;
    bool mLF_ON_GA;

    int mCommands;
    bool mMCCP_version_1;
    bool mMCCP_version_2;


    std::string mMudData;
    bool mIsTimerPosting;
    QTimer* mTimerLogin;
    QTimer* mTimerPass;
    QTime timeOffset;
    QTime mConnectionTime;
    int lastTimeOffset;
    bool enableATCP;
    bool enableGMCP;
    bool enableChannel102;
    QStringList messageStack;
    bool loadingReplay;
    QStringList mAcceptableEncodings;
};

#endif // MUDLET_CTELNET_H
