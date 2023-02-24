#ifndef MUDLET_CTELNET_H
#define MUDLET_CTELNET_H

/***************************************************************************
 *   Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com            *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2015 by Florian Scheel - keneanung@googlemail.com  *
 *   Copyright (C) 2015, 2017-2019, 2021-2022 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
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

// (1 of 2) This must be included before any Qt library tries to include
// windows.h which pulls in winsock.h to avoid (multiple):
// "#warning Please include winsock2.h before windows.h [-Wcpp]" warnings
#if defined(INCLUDE_WINSOCK2)
#include <winsock2.h>
#endif

#include "pre_guard.h"
#include <QElapsedTimer>
#include <QHostAddress>
#include <QHostInfo>
#include <QPointer>
#include <QStringList>
#if defined(QT_NO_SSL)
#include <QTcpSocket>
#else
#include <QSslSocket>
#endif
#include <QTime>
#include "post_guard.h"

#include <zlib.h>

#include <iostream>
#include <queue>
#include <string>

#if defined(Q_OS_WIN32)
#include <ws2tcpip.h>
#include "mstcpip.h"
#else
#include <sys/socket.h>
/*
 * The Linux documentation for setsockopt(2), indicates that "sys/types.h" is
 * optional for that OS but is suggested for portability with other OSes also
 * derived from BSD code - it is included in the corresponding FreeBSD manpage!
 */
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#endif

class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QTextCodec;
class QTextDecoder;
class QTextEncoder;
class QTimer;

class Host;
class dlgComposer;


const char TN_BELL = static_cast<char>(7);

const char TN_EOR = static_cast<char>(239);
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

const char TNSB_IS = 0;
const char TNSB_SEND = 1;


const char OPT_ECHO = 1;
const char OPT_STATUS = 5;
const char OPT_TIMING_MARK = 6;
const char OPT_TERMINAL_TYPE = 24;
const char OPT_EOR = 25;
const char OPT_NAWS = 31;
const char OPT_CHARSET = 42;
const char OPT_MSDP = 69; // http://tintin.sourceforge.net/msdp/
const char OPT_MSSP = static_cast<char>(70); // https://tintin.sourceforge.io/protocols/mssp/
const char OPT_COMPRESS = 85;
const char OPT_COMPRESS2 = 86;
const char OPT_MSP = 90;
const char OPT_MXP = 91;
const char OPT_102 = 102;
const char OPT_ATCP = static_cast<char>(200);
const char OPT_GMCP = static_cast<char>(201);

const char CHARSET_REQUEST = 1;
const char CHARSET_ACCEPTED = 2;
const char CHARSET_REJECTED = 3;
const char CHARSET_TTABLE_IS = 4;
const char CHARSET_TTABLE_REJECTED = 5;
const char CHARSET_TTABLE_ACK = 6;
const char CHARSET_TTABLE_NAK = 7;

const char MSSP_VAR = 1;
const char MSSP_VAL = 2;

const char MSDP_VAR = 1;
const char MSDP_VAL = 2;
const char MSDP_TABLE_OPEN = 3;
const char MSDP_TABLE_CLOSE = 4;
const char MSDP_ARRAY_OPEN = 5;
const char MSDP_ARRAY_CLOSE = 6;

class cTelnet : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(cTelnet)
    cTelnet(Host* pH, const QString&);
    ~cTelnet();
    void connectIt(const QString& address, int port);
    void reconnect();
    void disconnectIt();
    void abortConnection();
    // Second argument needs to be set false when sending password to prevent
    // it being sniffed by scripts/packages:
    bool sendData(QString& data, bool permitDataSendRequestEvent = true);
    void setATCPVariables(const QByteArray&);
    void setGMCPVariables(const QByteArray&);
    void setMSSPVariables(const QByteArray&);
    void setMSPVariables(const QByteArray&);
    bool isIPAddress(QString&);
    bool purgeMediaCache();
    void atcpComposerCancel();
    void atcpComposerSave(QString);
    void checkNAWS();
    void setAutoReconnect(bool status);
    void encodingChanged(const QByteArray&);
    void set_USE_IRE_DRIVER_BUGFIX(bool b) { mUSE_IRE_DRIVER_BUGFIX = b; }
    void recordReplay();
    bool loadReplay(const QString&, QString* pErrMsg = nullptr);
    void loadReplayChunk();
    bool isReplaying() { return loadingReplay; }
    void setChannel102Variables(const QString&);
    bool socketOutRaw(std::string& data);
    const QByteArray & getEncoding() const { return mEncoding; }
    QPair<bool, QString> setEncoding(const QByteArray&, bool saveValue = true);
    void postMessage(QString);
    const QByteArrayList & getEncodingsList() const { return mAcceptableEncodings; }
    QAbstractSocket::SocketError error();
    QString errorString();
#if !defined(QT_NO_SSL)
    QSslCertificate getPeerCertificate();
    QList<QSslError> getSslErrors();
#endif
    QByteArray decodeBytes(const char*);
    std::string encodeAndCookBytes(const std::string&);
    bool isCHARSETEnabled() const { return enableCHARSET; }
    bool isATCPEnabled() const { return enableATCP; }
    bool isGMCPEnabled() const { return enableGMCP; }
    bool isMSDPEnabled() const { return enableMSDP; }
    bool isMSSPEnabled() const { return enableMSSP; }
    bool isMSPEnabled() const { return enableMSP; }
    bool isChannel102Enabled() const { return enableChannel102; }
    void requestDiscordInfo();
    QString decodeOption(const unsigned char) const;
    QAbstractSocket::SocketState getConnectionState() const { return socket.state(); }
    std::tuple<QString, int, bool> getConnectionInfo() const;
    void setPostingTimeout(const int);
    int getPostingTimeout() const { return mTimeOut; }


    QMap<int, bool> supportedTelnetOptions;
    bool mResponseProcessed = true;
    double networkLatencyTime = 0.0;
    QElapsedTimer networkLatencyTimer;
    bool mAlertOnNewData = true;
    bool mGA_Driver = false;
    bool mFORCE_GA_OFF = false;
    QPointer<dlgComposer> mpComposer;
    QNetworkAccessManager* mpDownloader = nullptr;
    QProgressDialog* mpProgressDialog = nullptr;
    QString mServerPackage;
    QString mProfileName;


public slots:
    void slot_setDownloadProgress(qint64, qint64);
    void slot_replyFinished(QNetworkReply*);
    void slot_processReplayChunk();
    void slot_socketHostFound(QHostInfo);
    void slot_socketConnected();
    void slot_socketDisconnected();
    void slot_socketReadyToBeRead();
// Not used    void slot_socketError();
#if !defined(QT_NO_SSL)
    void slot_socketSslError(const QList<QSslError>&);
#endif
    void slot_timerPosting();
    void slot_send_login();
    void slot_send_pass();

signals:
    // Intended to signal status changes for other parts of application
    void signal_connecting(Host*);
    void signal_connected(Host*);
    void signal_disconnected(Host*);


private:
    cTelnet() = default;

    void processSocketData(char *data, int size);
    void initStreamDecompressor();
    int decompressBuffer(char*& in_buffer, int& length, char* out_buffer);
    void reset();

    void processTelnetCommand(const std::string& telnetCommand);
    void sendTelnetOption(char type, char option);
    void gotRest(std::string&);
    void gotPrompt(std::string&);
    void postData();
    void raiseProtocolEvent(const QString& name, const QString& protocol);
    void setKeepAlive(int socketHandle);
    void processChunks();
    void promptTlsConnectionAvailable();
    void sendNAWS(int x, int y);
    static std::pair<bool, bool> testReadReplayFile();


    QPointer<Host> mpHost;
#if defined(QT_NO_SSL)
    QTcpSocket socket;
#else
    QSslSocket socket;
#endif
    QHostAddress mHostAddress;
//    QTextCodec* incomingDataCodec;
    QTextCodec* mpOutOfBandDataIncomingCodec = nullptr;
    QTextCodec* outgoingDataCodec = nullptr;
//    QTextDecoder* incomingDataDecoder;
    QTextEncoder* outgoingDataEncoder = nullptr;
    QString hostName;
    int hostPort = 0;
    bool mWaitingForResponse = false;
    std::queue<int> mCommandQueue;

    z_stream mZstream = {};

    bool mNeedDecompression = false;
    std::string command;
    bool iac = false;
    bool iac2 = false;
    bool insb = false;
    // Set if we have negotiated the use of the option by us:
    bool myOptionState[256];
    // Set if he has negotiated the use of the option by him:
    bool hisOptionState[256];
    // Set if we have tried to negotiate the use of the option by us:
    bool announcedState[256];
    // Set if the Server tried to negotiate the use of the option by him:
    bool heAnnouncedState[256];
    // BUG: never set to be true - but seems to hold our intention to want to
    // enable our use of the option!
    bool triedToEnable[256];
    bool recvdGA = false;

    QString termType;
    QByteArray mEncoding;
    QTimer* mpPostingTimer = nullptr;
    // We do not directly adjust the interval for the above because doing so
    // while it is active changes the timerId which might have unforeseen
    // effects - so instead we change the following and the revised value is
    // then used the next time the timer is stopped and then started:
    int mTimeOut = 300;
    bool mUSE_IRE_DRIVER_BUGFIX = false;

    QNetworkReply* mpPackageDownloadReply = nullptr;

    int mCommands = 0;
    bool mMCCP_version_1 = false;
    bool mMCCP_version_2 = false;


    std::string mMudData;
    bool mIsTimerPosting = false;
    QTimer* mTimerLogin = nullptr;
    QTimer* mTimerPass = nullptr;
    QElapsedTimer mRecordingChunkTimer;
    QElapsedTimer mConnectionTimer;
    qint32 mRecordLastChunkMSecTimeOffset = 0;
    int mRecordingChunkCount = 0;
    bool mReplayHasFaultyFormat = false;
    bool enableCHARSET = false;
    bool enableATCP = false;
    bool enableGMCP = false;
    bool enableMSSP = false;
    bool enableMSDP = false;
    bool enableMSP = false;
    bool enableChannel102 = false;
    bool mDontReconnect = false;
    bool mAutoReconnect = false;
    QStringList messageStack;
    // True if THIS profile is playing a replay, does not know about any OTHER
    // active profile...
    bool loadingReplay = false;
    // Used to disable the TConsole ending messages if run from lua:
    bool mIsReplayRunFromLua = false;
    QByteArrayList mAcceptableEncodings;
    // Used to prevent more than one warning being shown in the event of a bad
    // encoding (when the user wants to use characters that cannot be encoded in
    // the current Server Encoding) - gets reset when the encoding is changed:
    bool mEncodingWarningIssued = false;
    // Same sort of thing if an encoder fails to be found/loaded:
    bool mEncoderFailureNoticeIssued = false;

    // Set if the current connection is via a proxy
    bool mConnectViaProxy = false;

    // server problem w/ not terminating IAC SB: only warn once
    bool mIncompleteSB = false;

    // Need to track the current width and height of the TMainConsole so that
    // we can send NAWS data when it changes:
    int mNaws_x = 0;
    int mNaws_y = 0;
};

#endif // MUDLET_CTELNET_H
