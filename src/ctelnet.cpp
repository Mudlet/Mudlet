/***************************************************************************
 *   Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com            *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2017-2018 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2015 by Florian Scheel - keneanung@googlemail.com       *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Michael Hupp - darksix@northfire.org            *
 *   Copyright (C) 2017 by Colton Rasbury - rasbury.colton@gmail.com       *
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


#include "Host.h"
#include "TBuffer.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TMap.h"
#include "dlgComposer.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QProgressDialog>
#include <QTextCodec>
#include <QTextEncoder>
#include "post_guard.h"

#define DEBUG

using namespace std;

char loadBuffer[100001];
int loadedBytes;
QDataStream replayStream;
QFile replayFile;


cTelnet::cTelnet(Host* pH)
: mResponseProcessed(true)
, networkLatency()
, mAlertOnNewData(true)
, mGA_Driver(false)
, mFORCE_GA_OFF(false)
, mpComposer(nullptr)
, mpHost(pH)
, mEncoding()
, mpPostingTimer(new QTimer(this))
, mUSE_IRE_DRIVER_BUGFIX(false)
, mLF_ON_GA(false)
, mCommands(0)
, mMCCP_version_1(false)
, mMCCP_version_2(false)
, mpProgressDialog()
, hostPort()
, networkLatencyMin()
, networkLatencyMax()
, mWaitingForResponse()
, mZstream()
, recvdGA()
, lastTimeOffset()
, enableATCP(false)
, enableGMCP(false)
, enableChannel102(false)
, loadingReplay(false)
, mIsReplayRunFromLua(false)
{
    mIsTimerPosting = false;
    mNeedDecompression = false;

    // initialize encoding to a sensible default - needs to be a different value
    // than that in the initialisation list so that it is processed as a change
    // to set up the initial encoder
    encodingChanged(QStringLiteral("UTF-8"));
    termType = QStringLiteral("Mudlet %1").arg(QStringLiteral(APP_VERSION));
    if (QByteArray(APP_BUILD).trimmed().length()) {
        termType.append(QStringLiteral(APP_BUILD));
    }

    iac = iac2 = insb = false;

    command = "";
    curX = 80;
    curY = 25;

    if (mAcceptableEncodings.isEmpty()) {
        mAcceptableEncodings << QStringLiteral("UTF-8");
        mAcceptableEncodings << QStringLiteral("GBK");
        mAcceptableEncodings << QStringLiteral("GB18030");
        mAcceptableEncodings << QStringLiteral("ISO 8859-1");
        mAcceptableEncodings << TBuffer::getComputerEncodingNames();
    }

    if (mFriendlyEncodings.isEmpty()) {
        mFriendlyEncodings << QStringLiteral("UTF-8");
        mFriendlyEncodings << QStringLiteral("GBK");
        mFriendlyEncodings << QStringLiteral("GB18030");
        mFriendlyEncodings << QStringLiteral("ISO 8859-1");
        mFriendlyEncodings << TBuffer::getFriendlyEncodingNames();
    }

    // initialize the socket
    connect(&socket, SIGNAL(connected()), this, SLOT(handle_socket_signal_connected()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(handle_socket_signal_disconnected()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(handle_socket_signal_readyRead()));

    // initialize telnet session
    reset();

    mpPostingTimer->setInterval(300); //FIXME
    connect(mpPostingTimer, SIGNAL(timeout()), this, SLOT(slot_timerPosting()));

    mTimerLogin = new QTimer(this);
    mTimerLogin->setSingleShot(true);
    connect(mTimerLogin, SIGNAL(timeout()), this, SLOT(slot_send_login()));

    mTimerPass = new QTimer(this);
    mTimerPass->setSingleShot(true);
    connect(mTimerPass, SIGNAL(timeout()), this, SLOT(slot_send_pass()));

    mpDownloader = new QNetworkAccessManager(this);
    connect(mpDownloader, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
}

void cTelnet::reset()
{
    //prepare option variables
    for (int i = 0; i < 256; i++) {
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
    if (loadingReplay) {
        // If we are doing a replay we had better abort it so that if we are
        // NOT the "last profile standing" the replay system gets reset for
        // another profile to use:
        loadingReplay = false;
        replayFile.close();
        qDebug() << "cTelnet::~cTelnet() INFO - A replay was in progress on this profile but has been aborted.";
        mudlet::self()->replayOver();
    }

    if (!messageStack.empty()) {
        qWarning("cTelnet::~cTelnet() Instance being destroyed before it could display some messages,\nmessages are:\n------------");
        foreach (QString message, messageStack) {
            qWarning("%s\n------------", qPrintable(message));
        }
    }
    socket.deleteLater();
}


void cTelnet::encodingChanged(const QString& encoding)
{
    // unicode carries information in form of single byte characters
    // and multiple byte character sequences.
    // the encoder and the decoder maintain translation state, i.e. they need to know the preceding
    // chars to make the correct decisions when translating into unicode and vice versa

    if (mEncoding != encoding) {
        mEncoding = encoding;
        // Not currently used as we do it by hand as we have to extract the data
        // from the telnet protocol and all the out-of-band stuff.  It might be
        // possible to use this in the future for non-UTF-8 traffic though.
//    incomingDataCodec = QTextCodec::codecForName(encoding.toLatin1().data());
//    incomingDataDecoder = incomingDataCodec->makeDecoder();

        outgoingDataCodec = QTextCodec::codecForName(encoding.toLatin1().data());
        // Do NOT create BOM on out-going text data stream!
        outgoingDataEncoder = outgoingDataCodec->makeEncoder(QTextCodec::IgnoreHeader);

        // No need to tell the TBuffer instance of the main TConsole for this
        // profile to change its QTextCodec to match as it now checks for
        // changes here on each incoming packet
    }
}

// returns the computer encoding name ("ISO 8859-5") given a human-friendly one ("ISO 8859-5 (Cyrillic)")
const QString& cTelnet::getComputerEncoding(const QString& encoding)
{
    return TBuffer::getComputerEncoding(encoding);
}

// returns the human-friendly one ("ISO 8859-5 (Cyrillic)") given a computer encoding name ("ISO 8859-5")
const QString& cTelnet::getFriendlyEncoding()
{
    int index = mAcceptableEncodings.indexOf(mEncoding);
    return mFriendlyEncodings.at(index);
}

// Need to set the encoding at the start but it does not need to be written out
// then. Return values are for Lua subsystem...!
// newEncoding must be EITHER: one of the FIXED non-translatable values in
// cTelnet::csmAcceptableEncodings
// OR "ASCII"
// OR an empty string (which means the same as the ASCII).
QPair<bool, QString> cTelnet::setEncoding(const QString& newEncoding, const bool isToStore)
{
    QString reportedEncoding = newEncoding;
    if (newEncoding.isEmpty() || newEncoding == QLatin1String("ASCII")) {
        reportedEncoding = QStringLiteral("ASCII");
        if (!mEncoding.isEmpty()) {
            // This will disable trancoding on:
            // input in TBuffer::translateToPlainText(...)
            // output in cTelnet::sendData(...)
            mEncoding.clear();
            if (isToStore) {
                mpHost->writeProfileData(QStringLiteral("encoding"), reportedEncoding);
            }
        }
    } else if (!mAcceptableEncodings.contains(newEncoding)) {
        // Not in list - so reject it
        return qMakePair(false,
                         QLatin1String(R"(Encoding ")") % newEncoding % QLatin1String("\" does not exist;\nuse one of the following:\n\"ASCII\", \"")
                                 % mAcceptableEncodings.join(QLatin1String(R"(", ")"))
                                 % QLatin1String(R"(".)"));
    } else if (mEncoding != newEncoding) {
        encodingChanged(newEncoding);
        if (isToStore) {
            mpHost->writeProfileData(QStringLiteral("encoding"), mEncoding);
        }
    }

    return qMakePair(true, QString());
}

void cTelnet::connectIt(const QString& address, int port)
{
    // wird an dieser Stelle gesetzt
    if (mpHost) {
        mUSE_IRE_DRIVER_BUGFIX = mpHost->mUSE_IRE_DRIVER_BUGFIX;
        mLF_ON_GA = mpHost->mLF_ON_GA;
        mFORCE_GA_OFF = mpHost->mFORCE_GA_OFF;
    }

    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
        connectIt(address, port);
        return;
    }

    hostName = address;
    hostPort = port;
    QString server = "[ INFO ]  - Looking up the IP address of server:" + address + ":" + QString::number(port) + " ...";
    postMessage(server);
    QHostInfo::lookupHost(address, this, SLOT(handle_socket_signal_hostFound(QHostInfo)));
}


void cTelnet::disconnect()
{
    socket.disconnectFromHost();
}

void cTelnet::handle_socket_signal_error()
{
    QString err = "[ ERROR ] - TCP/IP socket ERROR:" % socket.errorString();
    postMessage(err);
}

void cTelnet::slot_send_login()
{
    sendData(mpHost->getLogin());
}

void cTelnet::slot_send_pass()
{
    sendData(mpHost->getPass());
}

void cTelnet::handle_socket_signal_connected()
{
    reset();

    setKeepAlive(socket.socketDescriptor());

    QString msg = "[ INFO ]  - A connection has been established successfully.\n    \n    ";
    postMessage(msg);
    QString func = "onConnect";
    QString nothing = "";
    mpHost->mLuaInterpreter.call(func, nothing);
    mConnectionTime.start();
    if ((mpHost->getLogin().size() > 0) && (mpHost->getPass().size() > 0)) {
        mTimerLogin->start(2000);
        mTimerPass->start(3000);
    }

    TEvent event;
    event.mArgumentList.append(QStringLiteral("sysConnectionEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
}

void cTelnet::handle_socket_signal_disconnected()
{
    postData();

    TEvent event;
    event.mArgumentList.append(QStringLiteral("sysDisconnectionEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);

    QString msg;
    QTime timeDiff(0, 0, 0, 0);
    msg = QString("[ INFO ]  - Connection time: %1\n    ").arg(timeDiff.addMSecs(mConnectionTime.elapsed()).toString("hh:mm:ss.zzz"));
    mNeedDecompression = false;
    reset();
    QString err = "[ ALERT ] - Socket got disconnected.\nReason: " % socket.errorString();
    QString spacer = "    ";
    if (!mpHost->mIsGoingDown) {
        postMessage(spacer);
        postMessage(err);
        postMessage(msg);
    }
}

void cTelnet::handle_socket_signal_hostFound(QHostInfo hostInfo)
{
    if (!hostInfo.addresses().isEmpty()) {
        mHostAddress = hostInfo.addresses().constFirst();
        postMessage(tr("[ INFO ]  - The IP address of %1 has been found. It is: %2\n").arg(hostName, mHostAddress.toString()));
        postMessage(tr("[ INFO ]  - Trying to connect to %1: %2 ...\n").arg(mHostAddress.toString(), QString::number(hostPort)));
        socket.connectToHost(mHostAddress, hostPort);
    } else {
        socket.connectToHost(hostInfo.hostName(), hostPort);
        postMessage(tr("[ ERROR ] - Host name lookup Failure!\nConnection cannot be established.\nThe server name is not correct, not working properly,\nor your nameservers are not working properly."));
        return;
    }
}

bool cTelnet::sendData(QString& data)
{
    data.remove(QChar::LineFeed);

    TEvent event;
    event.mArgumentList.append(QStringLiteral("sysDataSendRequest"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(data);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);

    if (mpHost->mAllowToSendCommand) {
        string outData;
        if (!mEncoding.isEmpty()) {
            if (! outgoingDataCodec->canEncode(data)) {
                QString errorMsg = tr("[ WARN ] - Invalid characters in outgoing data, one or more characters cannot\n"
                                      "be encoded into the range that is acceptable for the character\n"
                                      "encoding that is currently set {\"%1\"} for the MUD Server."
                                      "It may not understand what is sent to it.").arg(mEncoding);
                postMessage(errorMsg);
            }
            // Even if there are bad characters - try to send it anyway...
            outData = outgoingDataEncoder->fromUnicode(data).constData();
        } else {
            // Plain, raw ASCII, we hope!
            // TODO: Moan if the user is trying to send non-ASCII characters out!
            outData = data.toStdString();
        }

        if (!mpHost->mUSE_UNIX_EOL) {
            outData.append("\r\n");
        } else {
            outData += "\n";
        }
        return socketOutRaw(outData);
    } else {
        mpHost->mAllowToSendCommand = true;
        return false;
    }
}


bool cTelnet::socketOutRaw(string& data)
{
    if (!socket.isWritable()) {
        return false;
    }
    int dataLength = data.length();
    int remlen = dataLength;

    do {
        int written = socket.write(data.data(), remlen);

        if (written == -1) {
            return false;
        }
        remlen -= written;
        dataLength += written;
    } while (remlen > 0);

    if (mGA_Driver) {
        mCommands++;
        if (mCommands == 1) {
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
    if (myOptionState[static_cast<int>(OPT_NAWS)]) {
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
        if (x1 == TN_IAC) {
            s += TN_IAC;
        }
        s += x2;
        if (x2 == TN_IAC) {
            s += TN_IAC;
        }
        s += y1;
        if (y1 == TN_IAC) {
            s += TN_IAC;
        }
        s += y2;
        if (y2 == TN_IAC) {
            s += TN_IAC;
        }

        s += TN_IAC;
        s += TN_SE;
        socketOutRaw(s);
    }
}

void cTelnet::sendTelnetOption(char type, char option)
{
#ifdef DEBUG_TELNET
    QString _type;
    switch ((quint8)type) {
    case 251:
        _type = "WILL";
        break;
    case 252:
        _type = "WONT";
        break;
    case 253:
        _type = "DO";
        break;
    case 254:
        _type = "DONT";
        break;
    default:
        _type = "ERROR wrong telnet type";
    };

    qDebug() << "CLIENT SENDING Telnet: " << _type << " " << (quint8)option;
#endif
    string cmd;
    cmd += TN_IAC;
    cmd += type;
    cmd += option;
    socketOutRaw(cmd);
}


void cTelnet::replyFinished(QNetworkReply* reply)
{
    mpProgressDialog->close();


    QFile file(mServerPackage);
    file.open(QFile::WriteOnly);
    file.write(reply->readAll());
    file.flush();
    file.close();
    mpHost->installPackage(mServerPackage, 0);
    QString packageName = mServerPackage.section("/", -1);
    packageName.replace(".zip", "");
    packageName.replace("trigger", "");
    packageName.replace("xml", "");
    packageName.replace(".mpackage", "");
    packageName.replace('/', "");
    packageName.replace('\\', "");
    packageName.replace('.', "");
    mpHost->mServerGUI_Package_name = packageName;
}

void cTelnet::setDownloadProgress(qint64 got, qint64 tot)
{
    mpProgressDialog->setRange(0, static_cast<int>(tot));
    mpProgressDialog->setValue(static_cast<int>(got));
}

void cTelnet::processTelnetCommand(const string& command)
{
    char ch = command[1];
#ifdef DEBUG_TELNET
    QString _type;
    switch ((quint8)ch) {
    case 239:
        _type = "TN_EOR";
        break;
    case 249:
        _type = "TN_GA";
        break;
    case 250:
        _type = "SB";
        break;
    case 251:
        _type = "WILL";
        break;
    case 252:
        _type = "WONT";
        break;
    case 253:
        _type = "DO";
        break;
    case 254:
        _type = "DONT";
        break;
    case 255:
        _type = "IAC";
        break;
    default:
        _type = QString::number((quint8)ch);
    };
    if (command.size() > 2) {
        qDebug() << "SERVER sends telnet signal (" << command.size() << "):" << _type << " + " << (quint8)command[2];
    } else {
        qDebug() << "SERVER sends telnet signal (" << command.size() << "):" << _type;
    }
#endif

    char option;
    switch (ch) {
    case TN_GA:
    case TN_EOR: {
        recvdGA = true;
        break;
    }
    case TN_WILL: {
        //server wants to enable some option (or he sends a timing-mark)...
        option = command[2];
        const auto idxOption = static_cast<int>(option);

        if (option == static_cast<char>(25)) //EOR support (END OF RECORD=TN_GA
        {
            qDebug() << "EOR enabled";
            sendTelnetOption(TN_DO, 25);
            break;
        }

        if (option == MSDP) //MSDP support
        {
            string _h;
            if (!mpHost->mEnableMSDP) {
                _h += TN_IAC;
                _h += TN_DONT;
                _h += MSDP; // disable MSDP per http://tintin.sourceforge.net/msdp/
                socketOutRaw(_h);
                qDebug() << "TELNET IAC DONT MSDP";
                break;
            } else {
                sendTelnetOption(TN_DO, 69);
                //need to send MSDP start sequence: IAC   SB MSDP MSDP_VAR "LIST" MSDP_VAL "COMMANDS" IAC SE
                //NOTE: MSDP does not need quotes for string/vals
                _h += TN_IAC;
                _h += TN_SB;
                _h += MSDP; //MSDP
                _h += 1;    //MSDP_VAR
                _h += "LIST";
                _h += 2; //MSDP_VAL
                _h += "COMMANDS";
                _h += TN_IAC;
                _h += TN_SE;
                socketOutRaw(_h);
                qDebug() << "TELNET IAC DO MSDP";
                raiseProtocolEvent("sysProtocolEnabled", "MSDP");
                break;
            }
        }
        if (option == static_cast<char>(200)) // ATCP support
        {
            //FIXME: this is a bug, some muds offer both atcp + gmcp
            if (mpHost->mEnableGMCP) {
                break;
            }

            qDebug() << "ATCP enabled";
            enableATCP = true;
            sendTelnetOption(TN_DO, static_cast<char>(200));

            string _h;
            _h += TN_IAC;
            _h += TN_SB;
            _h += static_cast<char>(200);
            _h += string("hello Mudlet ") + APP_VERSION + APP_BUILD + string("\ncomposer 1\nchar_vitals 1\nroom_brief 1\nroom_exits 1\nmap_display 1\n");
            _h += TN_IAC;
            _h += TN_SE;
            socketOutRaw(_h);
            raiseProtocolEvent("sysProtocolEnabled", "ATCP");
            break;
        }

        if (option == GMCP) {
            if (!mpHost->mEnableGMCP) {
                break;
            }

            enableGMCP = true;
            sendTelnetOption(TN_DO, GMCP);
            qDebug() << "GMCP enabled";

            string _h;
            _h = TN_IAC;
            _h += TN_SB;
            _h += GMCP;
            _h += string(R"(Core.Hello { "client": "Mudlet", "version": ")") + APP_VERSION + APP_BUILD + string(R"(" })");
            _h += TN_IAC;
            _h += TN_SE;

            socketOutRaw(_h);

            _h = TN_IAC;
            _h += TN_SB;
            _h += GMCP;
            _h += R"(Core.Supports.Set [ "Char 1", "Char.Skills 1", "Char.Items 1", "Room 1", "IRE.Rift 1", "IRE.Composer 1"])";
            _h += TN_IAC;
            _h += TN_SE;

            socketOutRaw(_h);

            raiseProtocolEvent("sysProtocolEnabled", "GMCP");
            break;
        }

        if (option == MXP) {
            if (!mpHost->mFORCE_MXP_NEGOTIATION_OFF) {
                sendTelnetOption(TN_DO, 91);

                raiseProtocolEvent("sysProtocolEnabled", "MXP");
                break;
            }
        }

        if (option == static_cast<char>(102)) // Aardwulf channel 102 support
        {
            qDebug() << "Aardwulf channel 102 support enabled";
            enableChannel102 = true;
            sendTelnetOption(TN_DO, 102);
            raiseProtocolEvent("sysProtocolEnabled", "channel102");
            break;
        }

        heAnnouncedState[idxOption] = true;
        if (triedToEnable[idxOption]) {
            hisOptionState[idxOption] = true;
            triedToEnable[idxOption] = false;
        } else {
            if (!hisOptionState[idxOption]) {
                //only if this is not set; if it's set, something's wrong wth the server
                //(according to telnet specification, option announcement may not be
                //unless explicitly requested)

                if ((option == OPT_STATUS) || (option == OPT_TERMINAL_TYPE) || (option == OPT_ECHO) || (option == OPT_NAWS)) {
                    sendTelnetOption(TN_DO, option);
                    hisOptionState[idxOption] = true;
                } else if ((option == OPT_COMPRESS) || (option == OPT_COMPRESS2)) {
                    //these are handled separately, as they're a bit special
                    if (mpHost->mFORCE_NO_COMPRESSION || ((option == OPT_COMPRESS) && (hisOptionState[static_cast<int>(OPT_COMPRESS2)]))) {
                        //protocol says: reject MCCP v1 if you have previously accepted MCCP v2...
                        sendTelnetOption(TN_DONT, option);
                        hisOptionState[idxOption] = false;
                        qDebug() << "Rejecting MCCP v1, because v2 has already been negotiated or FORCE COMPRESSION OFF is set to ON.";
                    } else {
                        sendTelnetOption(TN_DO, option);
                        hisOptionState[idxOption] = true;
                        //inform MCCP object about the change
                        if (option == OPT_COMPRESS) {
                            mMCCP_version_1 = true;
                            qDebug() << "MCCP v1 negotiated.";
                        } else {
                            mMCCP_version_2 = true;
                            qDebug() << "MCCP v2 negotiated!";
                        }
                    }
                } else if (supportedTelnetOptions.contains(option)) {
                    sendTelnetOption(TN_DO, option);
                    hisOptionState[idxOption] = true;
                } else {
                    sendTelnetOption(TN_DONT, option);
                    hisOptionState[idxOption] = false;
                }
            }
        }
        break;
    }

    case TN_WONT: {
//server refuses to enable some option...
#ifdef DEBUG
        qDebug() << "cTelnet::processTelnetCommand() TN_WONT command=" << (quint8)command[2];
#endif
        option = command[2];
        auto idxOption = static_cast<int>(option);
        if (triedToEnable[idxOption]) {
            hisOptionState[idxOption] = false;
            triedToEnable[idxOption] = false;
            heAnnouncedState[idxOption] = true;
        } else {
#ifdef DEBUG
            qDebug() << "cTelnet::processTelnetCommand() we dont accept his option because we didnt want it to be enabled";
#endif
            if (option == static_cast<char>(69)) // MSDP got turned off
            {
                raiseProtocolEvent("sysProtocolDisabled", "MSDP");
            }
            if (option == static_cast<char>(200)) // ATCP got turned off
            {
                raiseProtocolEvent("sysProtocolDisabled", "ATCP");
            }
            if (option == static_cast<char>(201)) // GMCP got turned off
            {
                raiseProtocolEvent("sysProtocolDisabled", "GMCP");
            }
            if (option == MXP) // MXP got turned off
            {
                raiseProtocolEvent("sysProtocolDisabled", "MXP");
            }
            if (option == static_cast<char>(102)) // channel 102 support
            {
                raiseProtocolEvent("sysProtocolDisabled", "channel102");
            }
            //send DONT if needed (see RFC 854 for details)
            if (hisOptionState[idxOption] || (heAnnouncedState[idxOption])) {
                sendTelnetOption(TN_DONT, option);
                hisOptionState[idxOption] = false;

                if (option == OPT_COMPRESS) {
                    mMCCP_version_1 = false;
                    qDebug() << "MCCP v1 disabled !";
                }
                if (option == OPT_COMPRESS2) {
                    mMCCP_version_2 = false;
                    qDebug() << "MCCP v2 disabled !";
                }
            }
            heAnnouncedState[idxOption] = true;
        }
        break;
    }

    case TN_DO: {
#ifdef DEBUG
        qDebug() << "telnet: server wants us to enable option:" << (quint8)command[2];
#endif
        //server wants us to enable some option
        option = command[2];
        auto idxOption = static_cast<quint8>(option);
        if (option == static_cast<char>(69) && mpHost->mEnableMSDP) // MSDP support
        {
            qDebug() << "TELNET IAC DO MSDP";
            sendTelnetOption(TN_WILL, 69);

            raiseProtocolEvent("sysProtocolEnabled", "MSDP");
            break;
        }
        if (option == static_cast<char>(200) && !mpHost->mEnableGMCP) // ATCP support, enable only if GMCP is off as GMCP is better
        {
            qDebug() << "TELNET IAC DO ATCP";
            enableATCP = true;
            sendTelnetOption(TN_WILL, static_cast<char>(200));
            raiseProtocolEvent("sysProtocolEnabled", "ATCP");
            break;
        }
        if (option == static_cast<char>(201) && mpHost->mEnableGMCP) // GMCP support
        {
            qDebug() << "TELNET IAC DO GMCP";
            enableGMCP = true;
            sendTelnetOption(TN_WILL, static_cast<char>(201));
            raiseProtocolEvent("sysProtocolEnabled", "GMCP");
            break;
        }
        if (option == MXP) // MXP support
        {
            sendTelnetOption(TN_WILL, 91);
            mpHost->mpConsole->print("\n<MXP support enabled>\n");
            raiseProtocolEvent("sysProtocolEnabled", "MXP");
            break;
        }
        if (option == static_cast<char>(102)) // channel 102 support
        {
            qDebug() << "TELNET IAC DO CHANNEL 102";
            enableChannel102 = true;
            sendTelnetOption(TN_WILL, 102);
            raiseProtocolEvent("sysProtocolEnabled", "channel102");
            break;
        }
#ifdef DEBUG
        qDebug() << "server wants us to enable telnet option " << (quint8)option << "(TN_DO + " << (quint8)option << ")";
#endif
        if (option == OPT_TIMING_MARK) {
            qDebug() << "OK we are willing to enable TIMING_MARK";
            //send WILL TIMING_MARK
            sendTelnetOption(TN_WILL, option);
        } else if (!myOptionState[255])
        //only if the option is currently disabled
        {
            if ((option == OPT_STATUS) || (option == OPT_NAWS) || (option == OPT_TERMINAL_TYPE)) {
                if (option == OPT_STATUS) {
                    qDebug() << "OK we are willing to enable telnet option STATUS";
                }
                if (option == OPT_TERMINAL_TYPE) {
                    qDebug() << "OK we are willing to enable telnet option TERMINAL_TYPE";
                }
                if (option == OPT_NAWS) {
                    qDebug() << "OK we are willing to enable telnet option NAWS";
                }
                sendTelnetOption(TN_WILL, option);
                myOptionState[idxOption] = true;
                announcedState[idxOption] = true;
            } else {
                qDebug() << "SORRY, we are NOT WILLING to enable this telnet option.";
                sendTelnetOption(TN_WONT, option);
                myOptionState[idxOption] = false;
                announcedState[idxOption] = true;
            }
        }
        if (option == OPT_NAWS) {
            //NAWS
            setDisplayDimensions();
        }
        break;
    }
    case TN_DONT: {
//only respond if value changed or if this option has not been announced yet
#ifdef DEBUG
        qDebug() << "cTelnet::processTelnetCommand() TN_DONT command=" << (quint8)command[2];
#endif
        option = command[2];
        if (option == static_cast<char>(69)) // MSDP got turned off
        {
            raiseProtocolEvent("sysProtocolDisabled", "MSDP");
        }
        if (option == static_cast<char>(200)) // ATCP got turned off
        {
            raiseProtocolEvent("sysProtocolDisabled", "ATCP");
        }
        if (option == static_cast<char>(201)) // GMCP got turned off
        {
            raiseProtocolEvent("sysProtocolDisabled", "GMCP");
        }
        if (option == MXP) // MXP got turned off
        {
            raiseProtocolEvent("sysProtocolDisabled", "MXP");
        }
        if (option == static_cast<char>(102)) // channel 102 support
        {
            raiseProtocolEvent("sysProtocolDisabled", "channel102");
        }
        int idxOption = option & 0xFF;
        if (myOptionState[idxOption] || (!announcedState[idxOption])) {
            sendTelnetOption(TN_WONT, option);
            announcedState[idxOption] = true;
        }
        myOptionState[idxOption] = false;
        break;
    }
    case TN_SB: {
        option = command[2];

        // MSDP
        if (option == static_cast<char>(69)) {
            QString _m = command.c_str();
            if (command.size() < 6) {
                return;
            }
            _m = _m.mid(3, command.size() - 5);
            mpHost->mLuaInterpreter.msdp2Lua(_m.toUtf8().data(), _m.length());
            return;
        }
        // ATCP
        if (option == static_cast<char>(200)) {
            QString _m = command.c_str();
            if (command.size() < 6) {
                return;
            }
            _m = _m.mid(3, command.size() - 5);
            setATCPVariables(_m);
            if (_m.startsWith("Auth.Request")) {
                string _h;
                _h += TN_IAC;
                _h += TN_SB;
                _h += static_cast<char>(200);
                _h += string("hello Mudlet ") + APP_VERSION + APP_BUILD + string("\ncomposer 1\nchar_vitals 1\nroom_brief 1\nroom_exits 1\n");
                _h += TN_IAC;
                _h += TN_SE;
                socketOutRaw(_h);
            }

            if (_m.startsWith("Client.GUI")) {
                if (!mpHost->mAcceptServerGUI) {
                    return;
                }

                QString version = _m.section('\n', 0);
                version.replace("Client.GUI ", "");
                version.replace('\n', " ");
                version = version.section(' ', 0, 0);

                int newVersion = version.toInt();
                if (mpHost->mServerGUI_Package_version != newVersion) {
                    QString _smsg = tr("<The server wants to upgrade the GUI to new version '%1'. Uninstalling old version '%2'>")
                                            .arg(QString::number(newVersion), QString::number(mpHost->mServerGUI_Package_version));
                    mpHost->mpConsole->print(_smsg.toLatin1().data());
                    mpHost->uninstallPackage(mpHost->mServerGUI_Package_name, 0);
                    mpHost->mServerGUI_Package_version = newVersion;
                }
                QString url = _m.section('\n', 1);
                QString packageName = url.section('/', -1);
                QString fileName = packageName;
                packageName.replace(".zip", "");
                packageName.replace("trigger", "");
                packageName.replace("xml", "");
                packageName.replace(".mpackage", "");
                packageName.replace('/', "");
                packageName.replace('\\', "");
                packageName.replace('.', "");
                mpHost->mpConsole->print("<Server offers downloadable GUI (url='");
                mpHost->mpConsole->print(url);
                mpHost->mpConsole->print("') (package='");
                mpHost->mpConsole->print(packageName);
                mpHost->mpConsole->print("')>\n");
                if (mpHost->mInstalledPackages.contains(packageName)) {
                    mpHost->mpConsole->print("<package is already installed>\n");
                    return;
                }

                mServerPackage = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), fileName);

                QNetworkReply* reply = mpDownloader->get(QNetworkRequest(QUrl(url)));
                mpProgressDialog = new QProgressDialog("downloading game GUI from server", "Abort", 0, 4000000, mpHost->mpConsole);
                connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(setDownloadProgress(qint64, qint64)));
                mpProgressDialog->show();
            }
            return;
        }

        // Original fix by CR, second revision by MH - To take out normal MCCP version 1 option and 2, no need for this. -MH
        // TODO: Remove these comments. Old boolean taken out for MCCP, and other options which were un-needed code. Rev.3 -MH //

        // GMCP
        if (option == static_cast<char>(201)) {
            QString rawPayload = command.c_str();
            if (command.size() < 6) {
                return;
            }
            // strip first 3 characters to get rid of <IAC><SB><201>
            // and strip the last 2 characters to get rid of <IAC><TN_SE>
            setGMCPVariables(rawPayload.mid(3, rawPayload.size() - 5));
            return;
        }

        if (option == static_cast<unsigned char>(102)) {
            QString _m = command.c_str();
            if (command.size() < 6) {
                return;
            }
            _m = _m.mid(3, command.size() - 5);
            setChannel102Variables(_m);
            return;
        }
        switch (option) //switch 2
        {
        case OPT_STATUS: {
            //see OPT_TERMINAL_TYPE for explanation why I'm doing this
            if (true) {
                qDebug() << "WARNING: FIXME #501";
                if (command[3] == TNSB_SEND) {
                    qDebug() << "WARNING: FIXME #504";
                    //request to send all enabled commands; if server sends his
                    //own list of commands, we just ignore it (well, he shouldn't
                    //send anything, as we do not request anything, but there are
                    //so many servers out there, that you can never be sure...)
                    string cmd;
                    cmd += TN_IAC;
                    cmd += TN_SB;
                    cmd += OPT_STATUS;
                    cmd += TNSB_IS;
                    for (short i = 0; i < 256; i++) {
                        if (myOptionState[i]) {
                            cmd += TN_WILL;
                            cmd += i;
                        }
                        if (hisOptionState[i]) {
                            cmd += TN_DO;
                            cmd += i;
                        }
                    }
                    cmd += TN_IAC;
                    cmd += TN_SE;
                    socketOutRaw(cmd);
                }
            }
            break;
        }

        case OPT_TERMINAL_TYPE: {
            if (myOptionState[static_cast<int>(OPT_TERMINAL_TYPE)]) {
                if (command[3] == TNSB_SEND) {
                    //server wants us to send terminal type; he can send his own type
                    //too, but we just ignore it, as we have no use for it...
                    string cmd;
                    cmd += TN_IAC;
                    cmd += TN_SB;
                    cmd += OPT_TERMINAL_TYPE;
                    cmd += TNSB_IS;
                    cmd += termType.toLatin1().data();
                    cmd += TN_IAC;
                    cmd += TN_SE;
                    socketOutRaw(cmd);
                }
            }
            //other cmds should not arrive, as they were not negotiated.
            //if they do, they are merely ignored
        }
        }; //end switch 2
        //other commands are simply ignored (NOP and such, see .h file for list)
    }
    }; //end switch 1
       // raise sysTelnetEvent for all unhandled protocols
       // EXCEPT TN_GA / TN_EOR, which come at the end of every transmission, for performance reaons
    if (command[1] != TN_GA && command[1] != TN_EOR) {
        auto type = static_cast<unsigned char>(command[1]);
        auto telnetOption = static_cast<unsigned char>(command[2]);
        QString msg = command.c_str();
        if (command.size() >= 6) {
            msg = msg.mid(3, command.size() - 5);
        }

        TEvent event;
        event.mArgumentList.append(QStringLiteral("sysTelnetEvent"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(QString::number(type));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        event.mArgumentList.append(QString::number(telnetOption));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        event.mArgumentList.append(msg);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(event);
    }
}

void cTelnet::setATCPVariables(const QString& msg)
{
    QString var;
    QString arg;
    bool single = true;
    if (msg.indexOf('\n') > -1) {
        var = msg.section("\n", 0, 0);
        arg = msg.section("\n", 1);
        single = false;
    } else {
        var = msg.section(" ", 0, 0);
        arg = msg.section(" ", 1);
    }

    if (var.startsWith("Client.Compose")) {
        QString title;
        if (!single) {
            title = var.section(" ", 1);
        } else {
            title = arg;
            arg = "";
        }
        if (mpComposer) {
            return;
        }
        mpComposer = new dlgComposer(mpHost);
        //FIXME
        if (arg.startsWith(" ")) {
            arg.remove(0, 1);
        }
        mpComposer->init(title, arg);
        mpComposer->raise();
        mpComposer->show();
        return;
    }
    var.remove('.');
    arg.remove('\n');
    int space = var.indexOf(' ');
    if (space > -1) {
        arg.prepend(" ");
        arg = arg.prepend(var.section(" ", 1));
        var = var.section(" ", 0, 0);
    }
    mpHost->mLuaInterpreter.setAtcpTable(var, arg);
    if (var.startsWith("RoomNum")) {
        if (mpHost->mpMap) {
            mpHost->mpMap->mRoomIdHash[mpHost->getName()] = arg.toInt();
            if (mpHost->mpMap->mpM && mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
                mpHost->mpMap->mpM->update();
                mpHost->mpMap->mpMapper->mp2dMap->update();
            }
        }
    }
}

void cTelnet::setGMCPVariables(const QString& msg)
{
    QString var;
    QString arg;
    if (msg.indexOf('\n') > -1) {
        var = msg.section(QChar::LineFeed, 0, 0);
        arg = msg.section(QChar::LineFeed, 1);
    } else {
        var = msg.section(QChar::Space, 0, 0);
        arg = msg.section(QChar::Space, 1);
    }

    if (msg.startsWith(QStringLiteral("Client.GUI"))) {
        if (!mpHost->mAcceptServerGUI) {
            return;
        }

        QString version = msg.section('\n', 0);
        version.remove(QStringLiteral("Client.GUI "));
        version.replace(QChar::LineFeed, QChar::Space);
        version = version.section(' ', 0, 0);

        int newVersion = version.toInt();
        if (mpHost->mServerGUI_Package_version != newVersion) {
            QString _smsg = tr("<The server wants to upgrade the GUI to new version '%1'. Uninstalling old version '%2'>")
                                    .arg(QString::number(newVersion), QString::number(mpHost->mServerGUI_Package_version));
            mpHost->mpConsole->print(_smsg.toLatin1().data());
            mpHost->uninstallPackage(mpHost->mServerGUI_Package_name, 0);
            mpHost->mServerGUI_Package_version = newVersion;
        }
        QString url = msg.section('\n', 1);
        QString packageName = url.section('/', -1);
        QString fileName = packageName;
        packageName.replace(".zip", "");
        packageName.replace("trigger", "");
        packageName.replace("xml", "");
        packageName.replace(".mpackage", "");
        packageName.replace('/', "");
        packageName.replace('\\', "");
        packageName.replace('.', "");
        mpHost->mpConsole->print("<Server offers downloadable GUI (url='");
        mpHost->mpConsole->print(url);
        mpHost->mpConsole->print("') (package='");
        mpHost->mpConsole->print(packageName);
        mpHost->mpConsole->print("')>\n");
        if (mpHost->mInstalledPackages.contains(packageName)) {
            mpHost->mpConsole->print("<package is already installed>\n");
            return;
        }

        mServerPackage = mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), fileName);

        QNetworkReply* reply = mpDownloader->get(QNetworkRequest(QUrl(url)));
        mpProgressDialog = new QProgressDialog("downloading game GUI from server", "Abort", 0, 4000000, mpHost->mpConsole);
        connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(setDownloadProgress(qint64, qint64)));
        mpProgressDialog->show();
        return;
    }

    mudDataChunks.push({DataChunkType::GMCP, msg.toStdString(), false});
    arg.remove('\n');
    // remove \r's from the data, as yajl doesn't like it
    arg.remove(QChar('\r'));
    mpHost->mLuaInterpreter.setGMCPTable(var, arg);
}

void cTelnet::setChannel102Variables(const QString& msg)
{
    // messages consist of 2 bytes only
    if (msg.size() < 2) {
        qDebug() << "ERROR: channel 102 message size != 2 bytes msg<" << msg << ">";
        return;
    } else {
        int _m = msg.at(0).toLatin1();
        int _a = msg.at(1).toLatin1();
        mpHost->mLuaInterpreter.setChannel102Table(_m, _a);
    }
}

void cTelnet::atcpComposerCancel()
{
    if (!mpComposer) {
        return;
    }
    mpComposer->close();
    mpComposer = nullptr;
    string msg = "*q\nno\n";
    socketOutRaw(msg);
}

void cTelnet::atcpComposerSave(QString txt)
{
    if (!mpHost->mEnableGMCP) {
        //olesetbuf \n <text>
        string _h;
        _h += TN_IAC;
        _h += TN_SB;
        _h += static_cast<char>(200);
        _h += "olesetbuf \n ";
        _h += txt.toLatin1().data();
        _h += '\n';
        _h += TN_IAC;
        _h += TN_SE;
        socketOutRaw(_h);
        _h.clear();
        _h += "*s\n";
        socketOutRaw(_h);
    } else {
        string _h;
        _h += TN_IAC;
        _h += TN_SB;
        _h += GMCP;
        _h += "IRE.Composer.SetBuffer";
        if (txt != "") {
            _h += "  ";
            _h += txt.toLatin1().data();
            _h += " ";
        }
        _h += TN_IAC;
        _h += TN_SE;
        socketOutRaw(_h);
        _h.clear();
        _h += "*s\n";
        socketOutRaw(_h);
    }
    if (!mpComposer) {
        return;
    }
    mpComposer->close();
    mpComposer = nullptr;
}

// Revamped to take additional [ WARN ], [ ALERT ] and [ INFO ] prefixes and to indent
// additional lines (ending with '\n') to last space character after "-"
// following prefix.
// Prefixes are made uppercase.
// Will store messages if the TConsole on which they are to be placed is not yet
// in existance as happens during startup, then pumps them out in order of
// arrival once a message arrives when the TConsole DOES exist.
void cTelnet::postMessage(QString msg)
{
    messageStack.append(msg);

    if (!mpHost->mpConsole) {
        // Console doesn't exist (yet), stack up messages until it does...
        return;
    }

    while (!messageStack.empty()) {
        while (messageStack.first().endsWith('\n')) { // Must strip off final line feeds as use that character for split() - will replace it later
            messageStack.first().chop(1);
        }

        QStringList body = messageStack.first().split(QChar('\n'));

        qint8 openBraceIndex = body.at(0).indexOf("[");
        qint8 closeBraceIndex = body.at(0).indexOf("]");
        qint8 hyphenIndex = body.at(0).indexOf("- ");
        if (openBraceIndex >= 0 && closeBraceIndex > 0 && closeBraceIndex < hyphenIndex) {
            quint8 prefixLength = hyphenIndex + 1;
            while (body.at(0).at(prefixLength) == ' ') {
                prefixLength++;
            }

            QString prefix = body.at(0).left(prefixLength).toUpper();
            QString firstLineTail = body.at(0).mid(prefixLength);
            body.removeFirst();
            if (prefix.contains("ERROR")) {
                mpHost->mpConsole->print(prefix, Qt::red, Qt::black);                                  // Bright Red on black
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(255, 255, 50), Qt::black); // Bright Yellow on black
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    // Fix for lua using tabs for indentation which was messing up justification:
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(255, 255, 50), Qt::black); // Bright Yellow on black
                }
            } else if (prefix.contains("LUA")) {
                mpHost->mpConsole->print(prefix, QColor(80, 160, 255), Qt::black);                    // Light blue on black
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(50, 200, 50), Qt::black); // Light green on black
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(200, 50, 50), Qt::black); // Red on black
                }
            } else if (prefix.contains("WARN")) {
                mpHost->mpConsole->print(prefix, QColor(0, 150, 190), Qt::black);
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(190, 150, 0), Qt::black); // Orange on black
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 150, 0), Qt::black);
                }
            } else if (prefix.contains("ALERT")) {
                mpHost->mpConsole->print(prefix, QColor(190, 100, 50), Qt::black);                     // Orangish on black
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(190, 190, 50), Qt::black); // Yellow on Black
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 190, 50), Qt::black); // Yellow on Black
                }
            } else if (prefix.contains("INFO")) {
                mpHost->mpConsole->print(prefix, QColor(0, 150, 190), Qt::black);                   // Cyan on black
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(0, 160, 0), Qt::black); // Light Green on Black
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(0, 160, 0), Qt::black); // Light Green on Black
                }
            } else if (prefix.contains("OK")) {
                mpHost->mpConsole->print(prefix, QColor(0, 160, 0), Qt::black);                        // Light Green on Black
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(190, 100, 50), Qt::black); // Orangish on black
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 100, 50), Qt::black); // Orangish on black
                }
            } else {                                                                                             // Unrecognised but still in a "[ something ] -  message..." format
                mpHost->mpConsole->print(prefix, QColor(190, 50, 50), QColor(190, 190, 190));                    // Foreground red, background bright grey
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(50, 50, 50), QColor(190, 190, 190)); //Foreground dark grey, background bright grey
                for (quint8 _i = 0; _i < body.size(); _i++) {
                    QString temp = body.at(_i);
                    temp.replace('\t', "        ");
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(50, 50, 50), QColor(190, 190, 190)); //Foreground dark grey, background bright grey
                }
            }
        } else {                                                                                      // No prefix found
            mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 190, 190), Qt::black); //Foreground bright grey, background black
        }
        messageStack.removeFirst();
    }
}

//forward data for further processing

// Patch for servers that need GA/EOR for prompt fixups
void cTelnet::applyGAFix()
{
    int j = 0;
    int s = mMudData.size();
    while (j < s) {
        // search for leading <LF> but skip leading ANSI control sequences
        if (mMudData[j] == 0x1B) {
            while (j < s) {
                if (mMudData[j] == 'm') {
                    goto NEXT;
                    break;
                }
                j++;
            }
        }
        if (mMudData[j] == '\n') {
            mMudData.erase(j, 1);
            break;
        } else {
            break;
        }
    NEXT:
        j++;
    }
}

void cTelnet::processPromptChunk(string& mud_data)
{
    mpPostingTimer->stop();
    mMudData += mud_data;
    if (mUSE_IRE_DRIVER_BUGFIX && mGA_Driver) {
        applyGAFix();
    }

    postData();
    mMudData = "";
    mIsTimerPosting = false;
}

void cTelnet::processChunk(string& mud_data)
{
    if (mud_data.empty()) {
        return;
    }

    if (!mGA_Driver) {
        size_t i = mud_data.rfind('\n');
        if (i != string::npos) {
            mMudData += mud_data.substr(0, i + 1);
            postData();
            mpPostingTimer->start();
            mIsTimerPosting = true;
            if (i + 1 < mud_data.size()) {
                mMudData = mud_data.substr(i + 1, mud_data.size());
            } else {
                mMudData = "";
            }
        } else {
            mMudData += mud_data;
            if (!mIsTimerPosting) {
                mpPostingTimer->start();
                mIsTimerPosting = true;
            }
        }

    } else {
        mMudData += mud_data;
        if (mUSE_IRE_DRIVER_BUGFIX && mGA_Driver) {
            applyGAFix();
        }
        postData();
        mMudData = "";
    }
}

void cTelnet::slot_timerPosting()
{
    if (!mIsTimerPosting) {
        return;
    }
    mMudData += "\r";
    postData();
    mMudData = "";
    mIsTimerPosting = false;
    mpHost->mpConsole->finalize();
}

void cTelnet::postData()
{
    if (mpHost->mpConsole) {
        mpHost->mpConsole->printOnDisplay(mMudData, true);
    }
    if (mAlertOnNewData) {
        QApplication::alert(mudlet::self(), 0);
    }
}

void cTelnet::initStreamDecompressor()
{
    mZstream.zalloc = Z_NULL;
    mZstream.zfree = Z_NULL;
    mZstream.opaque = Z_NULL;
    mZstream.avail_in = 0;
    mZstream.next_in = Z_NULL;

    inflateInit(&mZstream);
}

int cTelnet::decompressBuffer(char*& in_buffer, int& length, char* out_buffer)
{
    mZstream.avail_in = length;
    mZstream.next_in = (Bytef*)in_buffer;

    mZstream.avail_out = 100000;
    mZstream.next_out = (Bytef*)out_buffer;

    int zval = inflate(&mZstream, Z_SYNC_FLUSH);
    int outSize = 100000 - mZstream.avail_out;

    length = mZstream.avail_in;
    in_buffer = (char*)mZstream.next_in;

    if (zval == Z_STREAM_END) {
        inflateEnd(&mZstream);
        qDebug() << "recv Z_STREAM_END, ending compression";
        this->mNeedDecompression = false;

        hisOptionState[static_cast<int>(OPT_COMPRESS)] = false;
        hisOptionState[static_cast<int>(OPT_COMPRESS2)] = false;

        // zval should always be NULL on inflateEnd.  No need for an else block. MCCP Rev. 3 -MH //
        initStreamDecompressor();
        qDebug() << "Listening for new compression sequences";
        return -1;
    }
    return outSize;
}


void cTelnet::recordReplay()
{
    lastTimeOffset = 0;
    timeOffset.start();
}

bool cTelnet::loadReplay(const QString& name, QString* pErrMsg)
{
    replayFile.setFileName(name);
    if (replayFile.open(QIODevice::ReadOnly)) {
        if (!pErrMsg) {
            // Only post an information menu if initiated from GUI controls
            postMessage(tr("[ INFO ]  - Loading replay file:\n"
                           "\"%1\".")
                        .arg(name));
            mIsReplayRunFromLua = true;
        } else {
            mIsReplayRunFromLua = false;
        }
        replayStream.setDevice(&replayFile);
        loadingReplay = true;
        if (mudlet::self()->replayStart()) {
            // TODO: consider moving to a QTimeLine based system...?
            // This initiates the replay chunk reading/processing cycle:
            loadReplayChunk();
        } else {
            loadingReplay = false;
            if (pErrMsg) {
                *pErrMsg = QStringLiteral("cannot perform replay, another one seems to already be in progress; try again when it has finished.");
            } else {
                postMessage(tr("[ WARN ]  - Cannot perform replay, another one may already be in progress,\n"
                               "try again when it has finished."));
            }
            return false;
        }
    } else {
        if (pErrMsg) {
            // Call from lua case:
            *pErrMsg = QStringLiteral("cannot read file \"%1\", error message was: \"%2\".")
                    .arg(name, replayFile.errorString());
        } else {
            postMessage(tr("[ ERROR ] - Cannot read file \"%1\",\n"
                           "error message was: \"%2\".")
                        .arg(name, replayFile.errorString()));
        }
        return false;
    }

    return true;
}

void cTelnet::loadReplayChunk()
{
    if (!replayStream.atEnd()) {
        int offset;
        int amount;
        replayStream >> offset;
        replayStream >> amount;

        loadedBytes = replayStream.readRawData(loadBuffer, amount);
        // Previous use of loadedBytes + 1 caused a spurious character at end of
        // string display by a qDebug of the loadBuffer contents
        loadBuffer[loadedBytes] = '\0';
        mudlet::self()->mReplayTime = mudlet::self()->mReplayTime.addMSecs(offset);
        QTimer::singleShot(offset / mudlet::self()->mReplaySpeed, this, SLOT(slot_processReplayChunk()));
    } else {
        loadingReplay = false;
        replayFile.close();
        if (!mIsReplayRunFromLua) {
            postMessage(tr("[  OK  ]  - The replay has ended."));
        }
        mudlet::self()->replayOver();
    }
}


void cTelnet::slot_processReplayChunk()
{
    int datalen = loadedBytes;
    string cleandata = "";
    recvdGA = false;
    for (int i = 0; i < datalen; i++) {
        char ch = loadBuffer[i];
        if (iac || iac2 || insb || (ch == TN_IAC)) {
            if (!(iac || iac2 || insb) && (ch == TN_IAC)) {
                iac = true;
                command += ch;
            } else if (iac && (ch == TN_IAC) && (!insb)) {
                //2. seq. of two IACs
                iac = false;
                cleandata += ch;
                command = "";
            } else if (iac && (!insb) && ((ch == TN_WILL) || (ch == TN_WONT) || (ch == TN_DO) || (ch == TN_DONT))) {
                //3. IAC DO/DONT/WILL/WONT
                iac = false;
                iac2 = true;
                command += ch;
            } else if (iac2) {
                //4. IAC DO/DONT/WILL/WONT <command code>
                iac2 = false;
                command += ch;
                mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), false});
                cleandata = "";
                processTelnetCommand(command);
                command = "";
            } else if (iac && (!insb) && (ch == TN_SB)) {
                //5. IAC SB
                iac = false;
                insb = true;
                command += ch;
            } else if (iac && (!insb) && (ch == TN_SE)) {
                //6. IAC SE without IAC SB - error - ignored
                command = "";
                iac = false;
            } else if (insb) {
                //7. inside IAC SB
                command += ch;
                if (iac && (ch == TN_SE)) //IAC SE - end of subcommand
                {
                    mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), false});
                    cleandata = "";
                    processTelnetCommand(command);
                    command = "";
                    iac = false;
                    insb = false;
                }
                if (iac) {
                    iac = false;
                } else if (ch == TN_IAC) {
                    iac = true;
                }
            } else
            //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
            {
                iac = false;
                command += ch;
                mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), false});
                cleandata = "";
                processTelnetCommand(command);
                //this could have set receivedGA to true; we'll handle that later
                command = "";
            }
        } else {
            if (ch != '\r') {
                cleandata += ch;
            }
        }

        if (recvdGA) {
            mGA_Driver = true;
            if (mCommands > 0) {
                mCommands--;
                if (networkLatencyTime.elapsed() > 2000) {
                    mCommands = 0;
                }
            }

            if (mUSE_IRE_DRIVER_BUGFIX || mLF_ON_GA) {
                cleandata.push_back('\n'); //part of the broken IRE-driver bugfix to make up for broken \n-prepending in unsolicited lines, part #2 see line 628
            }
            recvdGA = false;
            mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), true});
            cleandata = "";
        }
    } //for

    processChunks();

    mpHost->mpConsole->finalize();
    if (loadingReplay) {
        loadReplayChunk();
    }
}

void cTelnet::handle_socket_signal_readyRead()
{
    mpHost->mInsertedMissingLF = false;

    if (mWaitingForResponse) {
        double time = networkLatencyTime.elapsed();
        networkLatency = time / 1000;
        mWaitingForResponse = false;
    }

    char in_bufferx[100010];
    char* in_buffer = in_bufferx;
    char out_buffer[100010];

    int amount = socket.read(in_buffer, 100000);
    in_buffer[amount + 1] = '\0';
    if (amount == -1) {
        return;
    }
    if (amount == 0) {
        return;
    }

    string cleandata = "";
    int datalen;
    do {
        datalen = amount;
        char* buffer = in_buffer;
        if (mNeedDecompression) {
            datalen = decompressBuffer(in_buffer, amount, out_buffer);
            buffer = out_buffer;
        }
        buffer[datalen] = '\0';
        if (mpHost->mpConsole->mRecordReplay) {
            mpHost->mpConsole->mReplayStream << timeOffset.elapsed() - lastTimeOffset;
            mpHost->mpConsole->mReplayStream << datalen;
            mpHost->mpConsole->mReplayStream.writeRawData(&buffer[0], datalen);
        }

        recvdGA = false;
        for (int i = 0; i < datalen; i++) {
            char ch = buffer[i];

            if (iac || iac2 || insb || (ch == TN_IAC)) {
                if (!(iac || iac2 || insb) && (ch == TN_IAC)) {
                    iac = true;
                    command += ch;
                } else if (iac && (ch == TN_IAC) && (!insb)) {
                    //2. seq. of two IACs
                    iac = false;
                    cleandata += ch;
                    command = "";
                } else if (iac && (!insb) && ((ch == TN_WILL) || (ch == TN_WONT) || (ch == TN_DO) || (ch == TN_DONT))) {
                    //3. IAC DO/DONT/WILL/WONT
                    iac = false;
                    iac2 = true;
                    command += ch;
                } else if (iac2) {
                    //4. IAC DO/DONT/WILL/WONT <command code>
                    iac2 = false;
                    command += ch;
                    mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), false});
                    cleandata = "";
                    processTelnetCommand(command);
                    command = "";
                } else if (iac && (!insb) && (ch == TN_SB)) {
                    //5. IAC SB
                    iac = false;
                    insb = true;
                    command += ch;
                } else if (iac && (!insb) && (ch == TN_SE)) {
                    //6. IAC SE without IAC SB - error - ignored
                    command = "";
                    iac = false;
                } else if (insb) {
                    if (!mNeedDecompression) {
                        // IAC SB COMPRESS WILL SE for MCCP v1 (unterminated invalid telnet sequence)
                        // IAC SB COMPRESS2 IAC SE for MCCP v2
                        if ((mMCCP_version_1 || mMCCP_version_2) && (!mNeedDecompression)) {
                            char _ch = buffer[i];
                            if ((_ch == OPT_COMPRESS) || (_ch == OPT_COMPRESS2)) {
                                bool _compress = false;
                                if ((i > 1) && (i + 2 < datalen)) {
                                    qDebug() << "checking mccp start seq...";
                                    if ((buffer[i - 2] == TN_IAC) && (buffer[i - 1] == TN_SB) && (buffer[i + 1] == TN_WILL) && (buffer[i + 2] == TN_SE)) {
                                        qDebug() << "MCCP version 1 starting sequence";
                                        _compress = true;
                                    }
                                    if ((buffer[i - 2] == TN_IAC) && (buffer[i - 1] == TN_SB) && (buffer[i + 1] == TN_IAC) && (buffer[i + 2] == TN_SE)) {
                                        qDebug() << "MCCP version 2 starting sequence";
                                        _compress = true;
                                    }
                                    qDebug() << (int)buffer[i - 2] << "," << (int)buffer[i - 1] << "," << (int)buffer[i] << "," << (int)buffer[i + 1] << "," << (int)buffer[i + 2];
                                }
                                if (_compress) {
                                    mNeedDecompression = true;
                                    // from this position in stream onwards, data will be compressed by zlib
                                    processChunk(cleandata);
                                    cleandata = "";
                                    initStreamDecompressor();
                                    buffer += i + 3; //bugfix: BenH
                                    int restLength = datalen - i - 3;
                                    if (restLength > 0) {
                                        datalen = decompressBuffer(buffer, restLength, out_buffer);
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
                    if (iac && (ch == TN_SE)) //IAC SE - end of subcommand
                    {
                        mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), false});
                        cleandata = "";
                        processTelnetCommand(command);
                        command = "";
                        iac = false;
                        insb = false;
                    }
                    if (iac) {
                        iac = false;
                    } else if (ch == TN_IAC) {
                        iac = true;
                    }
                } else
                //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
                {
                    iac = false;
                    command += ch;
                    mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), false});
                    cleandata = "";
                    processTelnetCommand(command);
                    //this could have set receivedGA to true; we'll handle that later
                    command = "";
                }
            } else {
                if (ch == TN_BELL) {
                    // flash taskbar for 3 seconds on the telnet bell
                    QApplication::alert(mudlet::self(), 3000);
                }
                if (ch != '\r' && ch != 0) {
                    cleandata += ch;
                }
            }
        MAIN_LOOP_END:;
            if (recvdGA) {
                if (!mFORCE_GA_OFF) //FIXME: wird noch nicht richtig initialisiert
                {
                    mGA_Driver = true;
                    if (mCommands > 0) {
                        mCommands--;
                        if (networkLatencyTime.elapsed() > 2000) {
                            mCommands = 0;
                        }
                    }
                    cleandata.push_back('\xff');
                    recvdGA = false;
                    mudDataChunks.push({DataChunkType::IN_BAND, std::move(cleandata), true});
                    cleandata = "";
                } else {
                    if (mLF_ON_GA) //TODO: reenable option in preferences
                    {
                        cleandata.push_back('\n');
                    }
                }
            }
        } //for
    } while (datalen == 100000);

    processChunks();
    mpHost->mpConsole->finalize();
    lastTimeOffset = timeOffset.elapsed();
}

void cTelnet::processChunks()
{
    while (!mudDataChunks.empty()) {
        auto& chunk = mudDataChunks.front();
        if (chunk.type == DataChunkType::GMCP) {
            QString data = QString::fromStdString(chunk.data);
            QString var;
            QString arg;
            if (data.indexOf('\n') > -1) {
                var = data.section(QChar::LineFeed, 0, 0);
                arg = data.section(QChar::LineFeed, 1);
            } else {
                var = data.section(QChar::Space, 0, 0);
                arg = data.section(QChar::Space, 1);
            }
            arg.remove('\n');
            arg.remove(QChar('\r'));
            mpHost->mLuaInterpreter.raiseInlineGMCPEvent(var, arg);
        } else if (chunk.type == DataChunkType::IN_BAND) {
            if (chunk.ends_with_prompt) {
                processChunk(chunk.data);
            } else {
                processPromptChunk(chunk.data);
            }
        }
        mudDataChunks.pop();
    }
}

void cTelnet::raiseProtocolEvent(const QString& name, const QString& protocol)
{
    TEvent event;
    event.mArgumentList.append(name);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(protocol);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
}

// credit: https://github.com/qflow/websockets
// Also see: https://stackoverflow.com/a/5435430/4805858
// particularly the comment: "Note that keepalive won't detect a failure until
// at least the configured keepalive_time + (keepalive_intrvl*keepalive_probes).
// I think by default if you don't change the settings this can default to over
// an hour!" – bdk Mar 25 '11 at 17:50
void cTelnet::setKeepAlive(int socketHandle)
{
    // Switch the keep-alive option on:
    int on = 1;
    // allow 75 seconds to set up connection {*nix-like OS default}:
    int init = 75;
    // send keepalive after 2 minutes of inactivity (after the init period)
    // {2 hours is default}:
    constexpr int timeout = 2 * 60;
    // send a keepalive packet every 75 seconds {*nix-like 0S default}:
    int interval = 75;
    // send up to 10 keepalive packets out - then disconnect if no response:
    int count = 10;
#if defined(Q_OS_WIN32)
    // Both Windows 32 and 64 bit despite the "32"

    // Windows is hardwired to use 10 for the count value (TCP_KEEPCNT) in Vista
    // and later.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd877220(v=vs.85).aspx
    Q_UNUSED(count)
    Q_UNUSED(init)
    struct tcp_keepalive
    {
        u_long onoff; // off = 0; on = not 0; default off
        u_long keepalivetime; // milliseconds, default = 7,200,000 = 2 hours
        u_long keepaliveinterval; // milliseconds, default = 1000 = 1 second
    } alive;
    alive.onoff = on;
    alive.keepalivetime = timeout * 1000;
    alive.keepaliveinterval = interval * 1000;
    DWORD dwBytesRet = 0;
    WSAIoctl(socketHandle, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &dwBytesRet, NULL, NULL);

#else // For OSes other than Windows:

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    setsockopt(socketHandle, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#else
    // FreeBSD always has the Keep-alive option enabled, so the above is not
    // needed
    Q_UNUSED(on)
#endif

    // The effect is that (on FreeBSD) "init" seconds is allowed to set up the
    // connection, then after another "timeout" minutes with no traffic a
    // keep-alive is sent - which should wake up the far end, if it does not
    // another one is sent after a further "interval" seconds and if NO response
    // is received after "count" of those keep alives then Mudlet will close the
    // socket itself - declaring the remote end dead... we are hoping that that
    // does not happen so that the FIRST keep-alive does what it is supposed to!

    // Time to establish connection on new, unconnected sockets, in seconds
#if defined(Q_OS_FREEBSD)
    // Only an option on FreeBSD:
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPINIT, &init, sizeof(init));
#else
    Q_UNUSED(init)
#endif

    // Start keepalives after this interval of idleness, in seconds:
#if defined(Q_OS_MACOS)
    // TCP_KEEPIDLE is TCP_KEEPALIVE on MacOs
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPALIVE, &timeout, sizeof(timeout));
#else
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(timeout));
#endif

    // Interval between keep-alives, in seconds:
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    // Number of failed keep alives before forcing a close:
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
#endif // defined(Q_OS_WIN32)
}
