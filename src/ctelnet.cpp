/***************************************************************************
 *   Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com            *
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014, 2017-2019, 2021-2022 by Stephen Lyons        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2015 by Florian Scheel - keneanung@googlemail.com       *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Michael Hupp - darksix@northfire.org            *
 *   Copyright (C) 2017 by Colton Rasbury - rasbury.colton@gmail.com       *
 *   Copyright (C) 2023 by Lecker Kebap - Leris@mudlet.org                 *
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
#include "TMainConsole.h"
#include "TMap.h"
#include "TMedia.h"
#include "GMCPAuthenticator.h"
#include "TTextCodec.h"
#include "TTextEdit.h"
#include "dlgComposer.h"
#include "dlgMapper.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
#endif

#include "pre_guard.h"
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QProgressDialog>
#include <QSslError>
#include "post_guard.h"

using namespace std::chrono_literals;


constexpr size_t BUFFER_SIZE = 100000L;
// TODO: https://github.com/Mudlet/Mudlet/issues/5780 (1 of 7) - investigate switching from using `char[]` to `std::array<char>`
char loadBuffer[BUFFER_SIZE + 1];
int loadedBytes;
QDataStream replayStream;
QFile replayFile;


cTelnet::cTelnet(Host* pH, const QString& profileName)
: mProfileName(profileName)
, mpHost(pH)
, mpPostingTimer(new QTimer(this))
{
    // initialize encoding to a sensible default - needs to be a different value
    // than that in the initialisation list so that it is processed as a change
    // to set up the initial encoder
    encodingChanged("UTF-8");
    termType = qsl("Mudlet " APP_VERSION);
    if (mudlet::self()->mAppBuild.trimmed().length()) {
        termType.append(mudlet::self()->mAppBuild);
    }

    command = "";
    // The raw string literals are QByteArrays now not QStrings:
    if (mAcceptableEncodings.isEmpty()) {
        mAcceptableEncodings << "UTF-8";
        mAcceptableEncodings << "EUC-KR";
        mAcceptableEncodings << "GBK";
        mAcceptableEncodings << "GB18030";
        mAcceptableEncodings << "BIG5";
        mAcceptableEncodings << "BIG5-HKSCS";
        mAcceptableEncodings << "ISO 8859-1";
        mAcceptableEncodings << TBuffer::getEncodingNames();
    }

    // initialize the socket after the Host initialisation is complete so we can access mSslTsl
    QTimer::singleShot(0, this, [this]() {
#if !defined(QT_NO_SSL)
        if (mpHost->mSslTsl) {
            connect(&socket, &QSslSocket::encrypted, this, &cTelnet::slot_socketConnected);
        } else {
            connect(&socket, &QAbstractSocket::connected, this, &cTelnet::slot_socketConnected);
        }
        connect(&socket, qOverload<const QList<QSslError>&>(&QSslSocket::sslErrors), this, &cTelnet::slot_socketSslError);
#else
        connect(&socket, &QAbstractSocket::connected, this, &cTelnet::slot_socketConnected);
#endif
        connect(&socket, &QAbstractSocket::disconnected, this, &cTelnet::slot_socketDisconnected);
        connect(&socket, &QIODevice::readyRead, this, &cTelnet::slot_socketReadyToBeRead);
    });


    // initialize telnet session
    reset();

    mpPostingTimer->setInterval(mTimeOut);
    connect(mpPostingTimer, &QTimer::timeout, this, &cTelnet::slot_timerPosting);

    mTimerLogin = new QTimer(this);
    mTimerLogin->setSingleShot(true);
    connect(mTimerLogin, &QTimer::timeout, this, &cTelnet::slot_send_login);

    mTimerPass = new QTimer(this);
    mTimerPass->setSingleShot(true);
    connect(mTimerPass, &QTimer::timeout, this, &cTelnet::slot_send_pass);

    mpDownloader = new QNetworkAccessManager(this);
    connect(mpDownloader, &QNetworkAccessManager::finished, this, &cTelnet::slot_replyFinished);
}

void cTelnet::reset()
{
    //reset telnet options state
    for (int i = 0; i < 256; ++i) {
        myOptionState[i] = false;
        hisOptionState[i] = false;
        announcedState[i] = false;
        heAnnouncedState[i] = false;
        triedToEnable[i] = false;
    }
    iac = false;
    iac2 = false;
    insb = false;
    // Ensure we do not think that the game server is echoing for us:
    mpHost->setRemoteEchoingActive(false);
    mGA_Driver = false;
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
#if defined (Q_OS_WINDOWS)
        // Windows does not seem to accept line-feeds in these strings:
        qWarning("cTelnet::~cTelnet() Instance being destroyed before it could display some messages,");
        qWarning("messages are:");
        qWarning("------------");
#else
        qWarning("cTelnet::~cTelnet() Instance being destroyed before it could display some messages,\nmessages are:\n------------");
#endif
        for (const auto& message : messageStack) {
#if defined (Q_OS_WINDOWS)
            qWarning("%s", qPrintable(message));
            qWarning("------------");
#else
            qWarning("%s\n------------", qPrintable(message));
#endif
        }
    }
    if (mpComposer) {
        mpComposer->deleteLater();
    }
    socket.deleteLater();
}

void cTelnet::cancelLoginTimers()
{
    if (mTimerLogin) {
        mTimerLogin->stop();
    }

    if (mTimerPass) {
        mTimerPass->stop();
    }
}

// This configures two out of three of the QTextCodec used by this profile:
// 1) A single or multi-byte encoder for all outgoing data
// 2) A single or multi-byte encoder for incoming OutOfBand data
// There is one more:
// 3) A multi-byte ONLY decoder for incoming InBand data, set in:
// the (void) TBuffer::encodingChanged(...) method and used in
// the (bool) TBuffer::processXXXSequence(...) methods {where XXX is "UTF8",
// "Big5" or "GB").
// We have a few substute TTextCodecs that are derived from the QTextCodec
// class and they all have a name the same as the ones we hoped that Qt would
// provide except they have a "M_" prefix. We, however hide that detail from the
// user so the value supplied as an argument MAY need to be matched against
// the prefixed name or not:
void cTelnet::encodingChanged(const QByteArray& requestedEncoding)
{
    // unicode carries information in form of single byte characters
    // and multiple byte character sequences.
    // the encoder and the decoder maintain translation state, i.e. they need to know the preceding
    // chars to make the correct decisions when translating into unicode and vice versa

    // If there is a match in mAcceptableEncodings with an "M_" prefix then we
    // need to add on that prefix:
    QByteArray encoding = mAcceptableEncodings.contains("M_" + requestedEncoding) ? "M_" + requestedEncoding : requestedEncoding;
    if (mEncoding != encoding) {
        mEncoding = encoding;
        mEncodingWarningIssued = false;
        mEncoderFailureNoticeIssued = false;
        // Not currently used as we do it by hand as we have to extract the data
        // from the telnet protocol and all the out-of-band stuff.  It might be
        // possible to use this in the future for non-UTF-8 traffic though.
//    incomingDataCodec = QTextCodec::codecForName(encoding);
//    incomingDataDecoder = incomingDataCodec->makeDecoder();

        outgoingDataCodec = QTextCodec::codecForName(encoding);
        // Do NOT create BOM on out-going text data stream!
        if (outgoingDataCodec) {
            outgoingDataEncoder = outgoingDataCodec->makeEncoder(QTextCodec::IgnoreHeader);
        } else {
            outgoingDataEncoder = nullptr;
        }

        if (!mEncoding.isEmpty() && mEncoding != "ASCII") {
            mpOutOfBandDataIncomingCodec = QTextCodec::codecForName(encoding);
            if (mpOutOfBandDataIncomingCodec) {
                qDebug().nospace() << "cTelnet::encodingChanged(" << encoding << ") INFO - Installing a codec for OOB protocols that can handle: " << mpOutOfBandDataIncomingCodec->aliases();
            } else {
                qWarning().nospace() << "cTelnet::encodingChanged(" << encoding << ") WARNING - Unable to locate a codec for OOB protocols that can handle: " << mEncoding;
            }

        } else if (mpOutOfBandDataIncomingCodec) {
            // Will get here if the encoding is ASCII (or empty which is treated
            // the same) and there is still an an encoder set:
            qDebug().nospace() << "cTelnet::encodingChanged(" << encoding << ") INFO - Uninstalling the codec for OOB protocols that can handle: " << mpOutOfBandDataIncomingCodec->aliases() << " as the new encoding setting of: "
                               << encoding << " does not need a dedicated one explicitly set...";
            mpOutOfBandDataIncomingCodec = nullptr;
        }

        // No need to tell the TBuffer instance of the main TConsole for this
        // profile to change its QTextCodec to match as it now checks for
        // changes here on each incoming packet
    }
}

#if !defined(QT_NO_SSL)
QSslCertificate cTelnet::getPeerCertificate()
{
    return socket.peerCertificate();
}

QList<QSslError> cTelnet::getSslErrors()
{
    return socket.sslHandshakeErrors();
}
#endif

QAbstractSocket::SocketError cTelnet::error()
{
    return socket.error();
}

QString cTelnet::errorString()
{
    return socket.errorString();
}

// newEncoding must be EITHER: one of the FIXED non-translatable values in
// cTelnet::csmAcceptableEncodings
// OR "ASCII"
// OR an empty string (which means the same as the ASCII).
// saveValue: if false do not bother to save the setting as a profile setting
// to the filesystem (because we have just read it from there!) otherwise, and
// by default, do save it:
QPair<bool, QString> cTelnet::setEncoding(const QByteArray& newEncoding, const bool saveValue)
{
    QByteArray reportedEncoding = newEncoding;
    bool updateNewEnviron = (mEncoding == "UTF-8" || newEncoding == "UTF-8");

    if (newEncoding.isEmpty() || newEncoding == "ASCII") {
        reportedEncoding = "ASCII";
        if (!mEncoding.isEmpty()) {
            // This will disable transcoding on:
            // input in TBuffer::translateToPlainText(...)
            // incoming OOB in TLuaInterpreter::encodeBytes(...)
            // output in cTelnet::sendData(...)
            mEncoding.clear();
            if (saveValue) {
                mpHost->writeProfileData(qsl("encoding"), reportedEncoding);
            }
        }
    } else if (!(mAcceptableEncodings.contains(newEncoding) || mAcceptableEncodings.contains("M_" + newEncoding))) {
        // Not in list (even with a "M_" prefix that indicates the relevant
        // QTextCodec is actually one of our own TTextCodecs) - so reject it
        // Since we want to hide the implementation detail that some of the
        // encoding names could have a "M_"  prefix we will need to preprocess
        // the list of encodings.
        // Since the mAcceptableEncodings list is unchanging once it has been
        // populated we only need to do this once and can save the results for
        // reuse - in hindsight this is undoing part of:
        // TBuffer::getEncodingNames() !
        static QByteArrayList fixedUpEncodings;
        if (fixedUpEncodings.isEmpty()) {
            fixedUpEncodings = mAcceptableEncodings;
            QMutableByteArrayListIterator itEncoding(fixedUpEncodings);
            while (itEncoding.hasNext()) {
                auto checkEncoding{itEncoding.next()};
                if (checkEncoding.left(2) == "M_") {
                    itEncoding.setValue(checkEncoding.mid(2));
                }
            }
        }
        return qMakePair(false,
                         QLatin1String(R"(Encoding ")") % newEncoding % QLatin1String("\" does not exist;\nuse one of the following:\n\"ASCII\", \"")
                                 % QLatin1String(fixedUpEncodings.join(R"(", ")"))
                                 % QLatin1String(R"(".)"));
    } else if (mEncoding != newEncoding && ("M_" + mEncoding) != newEncoding) {
        encodingChanged(newEncoding);

        if (saveValue) {
            mpHost->writeProfileData(qsl("encoding"), QLatin1String(mEncoding));
        }
    }

    sendInfoNewEnvironValue(qsl("CHARSET")); // Positioned here so we get ASCII updates too

    if (updateNewEnviron) {
        sendInfoNewEnvironValue(qsl("UTF-8"));
        sendInfoNewEnvironValue(qsl("MTTS"));
    }

    return qMakePair(true, QString());
}

void cTelnet::requestDiscordInfo()
{
    mudlet* pMudlet = mudlet::self();
    if (pMudlet->mDiscord.libraryLoaded()) {
        std::string data;
        data = TN_IAC;
        data += TN_SB;
        data += OPT_GMCP;
        data += std::string("External.Discord.Get");
        data += TN_IAC;
        data += TN_SE;

        // some games are buggy with MCCP on and require actual input before GMCP is processed
        data += "\n";

        socketOutRaw(data);
    }
}

void cTelnet::connectIt(const QString& address, int port)
{
    if (mpHost) {
        mUSE_IRE_DRIVER_BUGFIX = mpHost->mUSE_IRE_DRIVER_BUGFIX;
        mFORCE_GA_OFF = mpHost->mFORCE_GA_OFF;
        mCycleCountMTTS = 0;
        newEnvironVariablesSent.clear();

        if (mpHost->mUseProxy && !mpHost->mProxyAddress.isEmpty() && mpHost->mProxyPort != 0) {
            auto& proxy = mpHost->getConnectionProxy();
            socket.setProxy(*proxy);
            mConnectViaProxy = true;
        } else {
            socket.setProxy(QNetworkProxy::DefaultProxy);
            mConnectViaProxy = false;
        }
    }

    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
        connectIt(address, port);
        return;
    }

    emit signal_connecting(mpHost);

    hostName = address;
    hostPort = port;
    postMessage(tr("[ INFO ]  - Looking up the IP address of server: %1:%2 ...").arg(address, QString::number(port)));
    // don't use a compile-time slot for this: https://bugreports.qt.io/browse/QTBUG-67646
    QHostInfo::lookupHost(address, this, SLOT(slot_socketHostFound(QHostInfo)));
}

void cTelnet::reconnect()
{
    // if we've connected offline and wish to reconnect, the last
    // connection parameters aren't yet set
    if (hostName.isEmpty() && hostPort == 0) {
        connectIt(mpHost->getUrl(), mpHost->getPort());
    } else {
        connectIt(hostName, hostPort);
    }
}

void cTelnet::disconnectIt()
{
    mDontReconnect = true;
    socket.disconnectFromHost();

}

void cTelnet::abortConnection()
{
    mDontReconnect = true;
    socket.abort();
}

// Not used:
//void cTelnet::slot_socketError()
//{
//    QString err = tr("[ ERROR ] - TCP/IP socket ERROR:") % socket.errorString();
//    postMessage(err);
//}

void cTelnet::slot_send_login()
{
    if (!mpHost->getLogin().isEmpty()) {
        sendData(mpHost->getLogin());
    }
}

void cTelnet::slot_send_pass()
{
    if (!mpHost->getLogin().isEmpty() && !mpHost->getPass().isEmpty()) {
        sendData(mpHost->getPass(), false);
    }
}

void cTelnet::slot_socketConnected()
{
    QString msg;

    reset();
    setKeepAlive(socket.socketDescriptor());

    if (mpHost->mSslTsl)
    {
        msg = tr("[ INFO ]  - A secure connection has been established successfully.");
    } else {
        msg = tr("[ INFO ]  - A connection has been established successfully.");
    }
    msg.append(qsl("\n    \n    "));
    postMessage(msg);
    QString func = "onConnect";
    QString nothing = "";
    mpHost->mLuaInterpreter.call(func, nothing);
    mConnectionTimer.start();
    mTimerLogin->start(2s);
    mTimerPass->start(3s);

    emit signal_connected(mpHost);

    TEvent event {};
    event.mArgumentList.append(qsl("sysConnectionEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);
}

void cTelnet::slot_socketDisconnected()
{
    QString msg;
    TEvent event {};
    QString reason;
    QString spacer = "    ";
    bool sslerr = false;

    postData();

    emit signal_disconnected(mpHost);

    event.mArgumentList.append(qsl("sysDisconnectionEvent"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mpHost->raiseEvent(event);

    QTime timeDiff(0, 0, 0, 0);
    msg = tr("[ INFO ]  - Connection time: %1\n    ")
                  .arg(timeDiff.addMSecs(mConnectionTimer.elapsed())
                                /*:
                                This is the format to be used to show the profile connection time, it follows
                                the rules of the "QDateTime::toString(...)" function and may need
                                modification for some locales, e.g. France, Spain.
                                */
                               .toString(tr("hh:mm:ss.zzz")));
    mNeedDecompression = false;
    reset();

    if (!mpHost->isClosingDown()) {
        postMessage(spacer);

#if !defined(QT_NO_SSL)
        QList<QSslError> sslErrors = getSslErrors();
        QSslCertificate cert = socket.peerCertificate();

        if (mpHost->mSslIgnoreExpired) {
            sslErrors.removeAll(QSslError(QSslError::CertificateExpired, cert));
        }

        if (mpHost->mSslIgnoreSelfSigned) {
            sslErrors.removeAll(QSslError(QSslError::SelfSignedCertificate, cert));
        }

        sslerr = (sslErrors.count() > 0 && !mpHost->mSslIgnoreAll && mpHost->mSslTsl);

        if (sslerr) {
            mDontReconnect = true;

            for (int a = 0; a < sslErrors.count(); ++a) {
                reason.append(qsl("        %1\n").arg(QString(sslErrors.at(a).errorString())));
            }
            QString err = tr("[ ALERT ] - Socket got disconnected.\nReason: ") % reason;
            postMessage(err);
        } else {
#endif
            if (mDontReconnect) {
                reason = qsl("User Disconnected");
            } else {
                reason = socket.errorString();
            }
            if (reason == qsl("Error during SSL handshake: error:140770FC:SSL routines:SSL23_GET_SERVER_HELLO:unknown protocol")) {
                reason = tr("Secure connections aren't supported by this game on this port - try turning the option off.");
            }
            QString err = tr("[ ALERT ] - Socket got disconnected.\nReason: ") % reason;
            postMessage(err);
        }
        postMessage(msg);
#if !defined(QT_NO_SSL)
    }

    if (sslerr) {
        mudlet::self()->showOptionsDialog(qsl("tab_connection"));
    }
#endif

    if (mAutoReconnect && !mDontReconnect) {
        connectIt(hostName, hostPort);
    }
    mDontReconnect = false;
}

#if !defined(QT_NO_SSL)
void cTelnet::slot_socketSslError(const QList<QSslError>& errors)
{
    QSslCertificate cert = socket.peerCertificate();
    QList<QSslError> ignoreErrorList;

    if (mpHost->mSslIgnoreExpired) {
        ignoreErrorList << QSslError(QSslError::CertificateExpired, cert);
    }
    if (mpHost->mSslIgnoreSelfSigned) {
        ignoreErrorList << QSslError(QSslError::SelfSignedCertificate, cert);
    }

    if (mpHost->mSslIgnoreAll) {
        socket.ignoreSslErrors(errors);
    } else {
        socket.ignoreSslErrors(ignoreErrorList);
    }
}
#endif

void cTelnet::slot_socketHostFound(QHostInfo hostInfo)
{
#if !defined(QT_NO_SSL)
    if (mpHost->mSslTsl) {
        postMessage(qsl("%1\n").arg(tr("[ INFO ]  - Trying secure connection to %1: %2 ...").arg(hostInfo.hostName(), QString::number(hostPort))));
        socket.connectToHostEncrypted(hostInfo.hostName(), hostPort, QIODevice::ReadWrite);

    } else {
#endif
        if (!hostInfo.addresses().isEmpty()) {
            mHostAddress = hostInfo.addresses().constFirst();
            postMessage(qsl("%1\n").arg(tr("[ INFO ]  - The IP address of %1 has been found. It is: %2").arg(hostName, mHostAddress.toString())));
            if (!mConnectViaProxy) {
                postMessage(qsl("%1\n").arg(tr("[ INFO ]  - Trying to connect to %1:%2 ...").arg(mHostAddress.toString(), QString::number(hostPort))));
            } else {
                postMessage(qsl("%1\n").arg(tr("[ INFO ]  - Trying to connect to %1:%2 via proxy...").arg(mHostAddress.toString(), QString::number(hostPort))));
            }
            socket.connectToHost(mHostAddress, hostPort);
        } else {
            socket.connectToHost(hostInfo.hostName(), hostPort);
            postMessage(tr("[ ERROR ] - Host name lookup Failure!\n"
                           "Connection cannot be established.\n"
                           "The server name is not correct, not working properly,\n"
                           "or your nameservers are not working properly."));
            return;
        }
#if !defined(QT_NO_SSL)
    }
#endif
}

// This uses UTF-16BE encoded data but needs to be converted to the selected
// Mud Server encoding - it should NOT contain any Telnet protocol byte
// sequences:
bool cTelnet::sendData(QString& data, const bool permitDataSendRequestEvent)
{
    data.remove(QChar::LineFeed);

    if (Q_LIKELY(permitDataSendRequestEvent)) {
        TEvent event{};
        event.mArgumentList.append(qsl("sysDataSendRequest"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(data);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(event);
    }

    if (mpHost->mAllowToSendCommand) {
        std::string outData;
        auto errorMsgTemplate = "[ WARN ]  - Tried to send '%1' to the game, but it is unlikely to understand it.";
        if (!mEncoding.isEmpty()) {
            if (outgoingDataEncoder) {
                if ((!mEncodingWarningIssued) && (!outgoingDataCodec->canEncode(data))) {
                    QString errorMsg = tr(errorMsgTemplate,
                                          "%1 is the command that was sent to the game.").arg(data);
                    postMessage(errorMsg);
                    mEncodingWarningIssued = true;
                }
                // Even if there are bad characters - try to send it anyway...
                outData = outgoingDataEncoder->fromUnicode(data).constData();
            } else {
                if (!mEncoderFailureNoticeIssued) {
                    postMessage(tr("[ ERROR ] - Internal error, no codec found for current setting of {\"%1\"}\n"
                                   "so Mudlet cannot send data in that format to the Game Server. Please\n"
                                   "check to see if there is an alternative that the MUD and Mudlet can\n"
                                   "use. Mudlet will attempt to send the data using the ASCII encoding\n"
                                   "but will be limited to only unaccented characters of basic English.\n"
                                   "Note: this warning will only be issued once, until the encoding is\n"
                                   "changed.").arg(QLatin1String(mEncoding)));
                    mEncoderFailureNoticeIssued = true;
                }
                // Even if there are unusable characters - try to send it as ASCII ...
                outData = data.toStdString();
            }
        } else {
            // Plain, raw ASCII, we hope!
            for (int i = 0, total = data.size(); i < total; ++i) {
                if ((!mEncodingWarningIssued) && (data.at(i).row() || data.at(i).cell() > 127)){
                    QString errorMsg = tr(errorMsgTemplate,
                                          "%1 is the command that was sent to the game.").arg(data);
                    postMessage(errorMsg);
                    mEncodingWarningIssued = true;
                    break;
                }
            }
            // Even if there are bad characters - try to send it anyway...
            outData = data.toStdString();
        }

        if (!mpHost->mUSE_UNIX_EOL) {
            outData += "\r";
        }
        outData += "\n";

        // outData is using the selected Mud Server encoding here:
        // we need to cook any byte values from the encoding process that are
        // 0xff (assuming that there are no Telnet protocol sequences in here):
        outData = mudlet::replaceString(outData, "\xff", "\xff\xff");
        return socketOutRaw(outData);
    } else {

        mpHost->mAllowToSendCommand = true;
        return false;
    }
}

// Data is *expected* to be in the required MUD Server encoding on entry,
// of course plain ASCII *is* valid for all encodings including Big-5 and GBK,
// as we do NOT handle the weirdly different EBDIC!!!
bool cTelnet::socketOutRaw(std::string& data)
{
    // We were using socket.iswritable() but it was not clear that that was a
    // suitable way to check for an open, usable connection - whereas isvalid()
    // is true if the socket is valid and ready for use:
    if (!socket.isValid()) {
        return false;
    }
    std::size_t dataLength = data.length();
    std::size_t written = 0;

    do {
        // Must use the two-argument QAbstractSocket::write(...) because there
        // may be ASCII NUL characters in data and the first of those will
        // terminate the writing of the bytes following it in the single
        // argument method call:
        qint64 chunkWritten = socket.write(data.substr(written).data(), (dataLength - written));

        if (chunkWritten < 0) {
            // -1 is the sentinel (error) value but any other negative value
            // would not make sense and it would break the cast to the
            // (unsigned) std::size_t type in the next code fragment!
            return false;
        }

        written += static_cast<std::size_t>(chunkWritten);
    } while (written < dataLength);

    if (mGA_Driver) {
        ++mCommands;
        if (mCommands == 1) {
            mWaitingForResponse = true;
            networkLatencyTimer.restart();
        }
    }

    return true;
}

void cTelnet::checkNAWS()
{
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }
    // Use the smaller of the screen width or the wrapAt, then subtract the
    // width of the time stamps if they are showing:
    int naws_x = std::min(pHost->mScreenWidth, pHost->mWrapAt) - (pHost->mpConsole->showTimeStamps() ? mudlet::smTimeStampFormat.size() : 0);
    int naws_y = pHost->mScreenHeight;
    if ((naws_y > 0) && (myOptionState[static_cast<size_t>(OPT_NAWS)]) && ((mNaws_x != naws_x) || (mNaws_y != naws_y))) {
        sendNAWS(naws_x, naws_y);
        mNaws_x = naws_x;
        mNaws_y = naws_y;
    }
}

// https://www.rfc-editor.org/rfc/rfc1073
void cTelnet::sendNAWS(int width, int height)
{
    std::string message;
    message += TN_IAC; // Interpret As Command
    message += TN_SB;  // Sub-negotiation begins
    message += OPT_NAWS; // NAWS - Negotiate About Window Size
    char widthHighByte = static_cast<char>(width / 256);
    char widthLowByte = static_cast<char>(width % 256);
    char heightHighByte = static_cast<char>(height / 256);
    char heightLowByte = static_cast<char>(height % 256);
    // Double 0xff (IAC) byte values as required by protocol to prevent confusion with a real IAC
    message += widthHighByte;
    if (widthHighByte == TN_IAC) {
        message += TN_IAC;
    }
    message += widthLowByte;
    if (widthLowByte == TN_IAC) {
        message += TN_IAC;
    }
    message += heightHighByte;
    if (heightHighByte == TN_IAC) {
        message += TN_IAC;
    }
    message += heightLowByte;
    if (heightLowByte == TN_IAC) {
        message += TN_IAC;
    }

    message += TN_IAC; // Interpret As Command
    message += TN_SE;  // Sub-negotiation ends
    socketOutRaw(message);
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
    }

    qDebug().noquote().nospace() << "WE send telnet IAC " << _type << " " << decodeOption(option);
#endif
    std::string output;
    output += TN_IAC;
    output += type;
    output += option;
    // This will be unaffected by Mud Server encoding:
    socketOutRaw(output);
}


void cTelnet::slot_replyFinished(QNetworkReply* reply)
{
    mpProgressDialog->close();

    if (reply != mpPackageDownloadReply) {
        qWarning().nospace().noquote() << "cTelnet::slot_replyFinished(QNetworkReply*) ERROR - download finished, but it wasn't the one we are expecting";
        reply->deleteLater();
    } else {
        // don't process if download was aborted
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            mpPackageDownloadReply = nullptr;
            return;
        }

        QSaveFile file(mServerPackage);
        file.open(QFile::WriteOnly);
        file.write(reply->readAll());
        if (!file.commit()) {
            qDebug() << "cTelnet::slot_replyFinished: error downloading package: " << file.errorString();
        }
        reply->deleteLater();
        mpPackageDownloadReply = nullptr;
        mpHost->installPackage(mServerPackage, enums::PackageModuleType::Package);
        QString packageName = mServerPackage.section("/", -1);
        packageName.remove(QLatin1String(".zip"), Qt::CaseInsensitive);
        packageName.remove(QLatin1String(".trigger"), Qt::CaseInsensitive);
        packageName.remove(QLatin1String(".xml"), Qt::CaseInsensitive);
        packageName.remove(QLatin1String(".mpackage"), Qt::CaseInsensitive);
        packageName.remove(QLatin1Char('/'));
        packageName.remove(QLatin1Char('\\'));
        mpHost->mServerGUI_Package_name = packageName;
    }
}

void cTelnet::slot_setDownloadProgress(qint64 got, qint64 tot)
{
    mpProgressDialog->setRange(0, static_cast<int>(tot));
    mpProgressDialog->setValue(static_cast<int>(got));
}

// Helper to identify which protocol is being negotiated!
QString cTelnet::decodeOption(const unsigned char ch) const
{
    // From http://www.iana.org/assignments/telnet-options/telnet-options.xhtml
    // and other places:
    switch (ch) {
    // Official:
    case 0:     return QLatin1String("BINARY (0)");
    case 1:     return QLatin1String("ECHO (1)");
    case 2:     return QLatin1String("RECONNECTION (2)");
    case 3:     return QLatin1String("SUPPRESS_GO_AHEAD (3)");
    case 4:     return QLatin1String("APROX_MSG_SIZE (4)");
    case 5:     return QLatin1String("STATUS (5)");
    case 6:     return QLatin1String("TIMING_MARK (6)");
    case 7:     return QLatin1String("REMOTE_CTRL_TRANS_AND_ECHO (7)");
    case 8:     return QLatin1String("OUTPUT_L_WIDTH (8)");
    case 9:     return QLatin1String("OUTPUT_P_SIZE (9)");
    case 10:    return QLatin1String("OUTPUT_CR_DISPOSITION (10)");
    case 11:    return QLatin1String("OUTPUT_HTAB_STOPS (11)");
    case 12:    return QLatin1String("OUTPUT_HTAB_DISPOSITION (12)");
    case 13:    return QLatin1String("OUTPUT_FF_DISPOSITION (13)");
    case 14:    return QLatin1String("OUTPUT_VTAB_STOPS (14)");
    case 15:    return QLatin1String("OUTPUT_VTAB_DISPOSITION (15)");
    case 16:    return QLatin1String("OUTPUT_LF_DISPOSITION (16)");
    case 17:    return QLatin1String("EXTENDED_ASCII (17)");
    case 18:    return QLatin1String("LOGOUT (18)");
    case 19:    return QLatin1String("BYTE_MACRO (19)");
    case 20:    return QLatin1String("DATA_ENTRY_TERMINAL (20)");
    case 21:    return QLatin1String("SUPDUP (21)");
    case 22:    return QLatin1String("SUPDUP_OUTPUT (22)");
    case 23:    return QLatin1String("SEND_LOCATION (23)");
    case 24:    return QLatin1String("TTYPE (24)");
    case 25:    return QLatin1String("EOR (25)");
    case 26:    return QLatin1String("TACACS_USER_ID (26)");
    case 27:    return QLatin1String("OUTPUT_MARKING (27)");
    case 28:    return QLatin1String("TERMINAL_LOCATION_NUMBER (28)");
    case 29:    return QLatin1String("TELNET_3270_REGIME (29)");
    case 30:    return QLatin1String("X3_PAD (30)");
    case 31:    return QLatin1String("NAWS (31)");
    case 32:    return QLatin1String("TERMINAL_SPEED (32)");
    case 33:    return QLatin1String("REMOTE_FLOW_CONTROL (33)");
    case 34:    return QLatin1String("LINEMODE (34)");
    case 35:    return QLatin1String("X_DISPLAY_LOCATION (35)");
    case 36:    return QLatin1String("ENVIRONMENT_OPTION (36)");
    case 37:    return QLatin1String("AUTHENTICATION_OPTIOM (37)");
    case 38:    return QLatin1String("ENCRYPTION_OPTION (38)");
    case 39:    return QLatin1String("NEW_ENVIRONMENT_OPTION (39)");
    case 40:    return QLatin1String("TN3270E (40)");
    case 41:    return QLatin1String("XAUTH (41)");
    case 42:    return QLatin1String("CHARSET (42)");
    case 43:    return QLatin1String("TELNET_REMOTE_SERIAL_PORT (43)");
    case 44:    return QLatin1String("COM_PORT_CONTROL_OPTION (44)");
    case 45:    return QLatin1String("TELNET_SUPPRESS_LOCAL_ECHO (45)");
    case 46:    return QLatin1String("TELNET_START_TLS (46)");
    case 47:    return QLatin1String("KERMIT (47)");
    case 48:    return QLatin1String("SEND_URL (48)");
    case 49:    return QLatin1String("FORWARD_X (49)");

    // Unofficial:
    case 69:    return QLatin1String("MSDP (69)");
    case 70:    return QLatin1String("MSSP (70)");

    case 85:    return QLatin1String("MCCP (85)");
    case 86:    return QLatin1String("MCCP2 (86)");

    case 90:    return QLatin1String("MSP (90)");
    case 91:    return QLatin1String("MXP (91)");

    case 93:    return QLatin1String("ZENITH (93)");

    case 102:   return QLatin1String("AARDWULF (102)");

    // Official:
    case 138:   return QLatin1String("TELOPT_PRAGRMA_LOGON (138)");
    case 139:   return QLatin1String("TELOPT_SSPI_LOGON (139)");
    case 140:   return QLatin1String("TELOPT_PRAGMA_HEARTBEAT (140)");

    // Unofficial:
    case 200:   return QLatin1String("ATCP (200)");
    case 201:   return QLatin1String("GMCP (201)");

    // Official:
    case 255:   return QLatin1String("EXTENDED_OPTIONS_LIST (255)");
    default:
        return qsl("UNKNOWN (%1)").arg(ch, 3);
    }
}

std::tuple<QString, int, bool> cTelnet::getConnectionInfo() const
{
    // intentionally simplify connection state to a boolean
    const bool connected = socket.state() == QAbstractSocket::ConnectedState;

    if (hostName.isEmpty() && hostPort == 0) {
        return {mpHost->getUrl(), mpHost->getPort(), connected};
    } else {
        return {hostName, hostPort, connected};
    }
}

// escapes data to be send over NEW ENVIRON and MNES
QByteArray cTelnet::prepareNewEnvironData(const QString &arg)
{
    QString ret = arg;

    ret.replace(TN_IAC, qsl("%1%2").arg(TN_IAC, TN_IAC));
    ret.replace(NEW_ENVIRON_ESC, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_ESC));
    ret.replace(NEW_ENVIRON_VAL, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_VAL));
    ret.replace(NEW_ENVIRON_USERVAR, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_USERVAR));
    ret.replace(NEW_ENVIRON_VAR, qsl("%1%2").arg(NEW_ENVIRON_ESC, NEW_ENVIRON_VAR));

    return ret.toLatin1().constData();
}

QString cTelnet::getNewEnvironValueUser()
{
    return !mpHost->getLogin().isEmpty() ? mpHost->getLogin().trimmed() : QString();
}

QString cTelnet::getNewEnvironValueSystemType()
{
    QString systemType;

    // "SYSTEMTYPE" Inspired by https://www.rfc-editor.org/rfc/rfc1340.txt
    // Ordering redone to follow general format of TLuaInterpreter::getOs()
#if defined(Q_OS_CYGWIN)
    // Try for this one before Q_OS_WINDOWS as both are likely to be defined on
    // a Cygwin platform
    systemType = qsl("CYGWIN");
#elif defined(Q_OS_WINDOWS)
    // This is defined on BOTH Win32 and Win64 hosts - but it reflects
    // the build machine rather than the run-time one and our published
    // builds are actually 32-bit ones that can run on either. If we
    // really wanted to distinguish between the two bit-nesses we'd have
    // to do that at run-time - and we can probably leave off doing that
    // until we officially publish 64 bit builds specifically for Win64
    // machines:
    systemType = qsl("WIN32");
#elif (defined(Q_OS_MACOS))
    systemType = qsl("MACOS");
#elif defined(Q_OS_LINUX)
    systemType = qsl("LINUX");
#elif defined(Q_OS_HURD)
    systemType = qsl("HURD");
#elif (defined(Q_OS_FREEBSD_KERNEL))
    // Defined for BOTH Debian kFreeBSD hybrid with a GNU userland and
    // main FreeBSD so it must be after a Q_OS_FREEBSD check if we needed
    // to tell the different; OTOH only a Debian packager for this, now
    // obsolete hybrid would want to worry about this!
    systemType = qsl("FREEBSD");
#elif defined(Q_OS_NETBSD)
    systemType = qsl("NETBSD");
#elif defined(Q_OS_OPENBSD)
    systemType = qsl("OPENBSD");
#elif defined(Q_OS_BSD4)
    // Generic *nix - must be before unix and after other more specific results
    systemType = qsl("BSD4");
#elif defined(Q_OS_UNIX)
    systemType = qsl("UNIX");
#endif

    return systemType.isEmpty() ? QString(): systemType;
}

QString cTelnet::getNewEnvironCharset()
{
    const QString charsetEncoding = getEncoding();

    return !charsetEncoding.isEmpty() ? charsetEncoding : qsl("ASCII");
}

QString cTelnet::getNewEnvironClientName()
{
    return qsl("MUDLET");
}

QString cTelnet::getNewEnvironClientVersion()
{
    QString clientVersion = APP_VERSION;
    static const auto allInvalidCharacters = QRegularExpression(qsl("[^A-Z,0-9,-,\\/]"));
    static const auto multipleHyphens = QRegularExpression(qsl("-{2,}"));

    if (auto build = mudlet::self()->mAppBuild; build.trimmed().length()) {
        clientVersion.append(build);
    }

    /*
    * The valid characters for termType are more restricted than being ASCII
    * from https://tools.ietf.org/html/rfc1010 (page 29):
    * "A terminal names may be up to 40 characters taken from the set of uppercase
    * letters, digits, and the two punctuation characters hyphen and slash.  It must
    * start with a letter, and end with a letter or digit."
    */
    clientVersion = clientVersion.toUpper()
                                        .replace(QChar('.'), QChar('/'))
                                        .replace(QChar::Space, QChar('-'))
                                        .replace(allInvalidCharacters, QChar('-'))
                                        .replace(multipleHyphens, QChar('-'))
                                        .left(40);

    for (int i = clientVersion.size() - 1; i >= 0; --i) {
        if (clientVersion.at(i).isLetterOrNumber()) {
            clientVersion = clientVersion.left(i + 1);
            break;
        }
    }

    return clientVersion;
}

QString cTelnet::getNewEnvironTerminalType()
{
    return qsl("ANSI-TRUECOLOR");
}

QString cTelnet::getNewEnvironMTTS()
{
    int terminalStandards = MTTS_STD_ANSI|MTTS_STD_256_COLORS|MTTS_STD_OSC_COLOR_PALETTE|MTTS_STD_TRUECOLOR;

    if (getEncoding() == "UTF-8") {
        terminalStandards |= MTTS_STD_UTF_8;
    }

    if (mpHost->mAdvertiseScreenReader) {
        terminalStandards |= MTTS_STD_SCREEN_READER;
    }

    if (mpHost->mEnableMNES && !mpHost->mForceNewEnvironNegotiationOff) {
        terminalStandards |= MTTS_STD_MNES;
    }

#if !defined(QT_NO_SSL)
    terminalStandards |= MTTS_STD_SSL;
#endif

    return qsl("%1").arg(terminalStandards);
}

QString cTelnet::getNewEnvironANSI()
{
    return qsl("1");
}

QString cTelnet::getNewEnvironVT100()
{
    return qsl("0");
}

QString cTelnet::getNewEnviron256Colors()
{
    return qsl("1");
}

QString cTelnet::getNewEnvironUTF8()
{
    return getEncoding() == "UTF-8" ? qsl("1") : qsl("0");
}

QString cTelnet::getNewEnvironOSCColorPalette()
{
    return qsl("1");
}

QString cTelnet::getNewEnvironScreenReader()
{
    return mpHost->mAdvertiseScreenReader ? qsl("1") : qsl("0");
}

QString cTelnet::getNewEnvironTruecolor()
{
    return qsl("1");
}

QString cTelnet::getNewEnvironTLS()
{
#if !defined(QT_NO_SSL)
    return qsl("1");
#else
    return qsl("0");
#endif
}

QString cTelnet::getNewEnvironLanguage()
{
    return mudlet::self()->getInterfaceLanguage();
}

QString cTelnet::getNewEnvironWordWrap()
{
    return qsl("%1").arg(mpHost->mWrapAt);
}

QMap<QString, QPair<bool, QString>> cTelnet::getNewEnvironDataMap()
{
    QMap<QString, QPair<bool, QString>> newEnvironDataMap;
    const bool isUserVar = true;

    // Per https://tintin.mudhalla.net/protocols/mnes/, the variables are limited to the following only.
    // * These will be be requested with NEW_ENVIRON_VAR for the MNES protocol
    // * "IPADDRESS" Intentionally not implemented by Mudlet Makers
    // * These will be used by NEW_ENVIRON as well and be requested with NEW_ENVIRON_USERVAR
    newEnvironDataMap.insert(qsl("CHARSET"), qMakePair(isUserVar, getNewEnvironCharset()));
    newEnvironDataMap.insert(qsl("CLIENT_NAME"), qMakePair(isUserVar, getNewEnvironClientName()));
    newEnvironDataMap.insert(qsl("CLIENT_VERSION"), qMakePair(isUserVar, getNewEnvironClientVersion()));
    newEnvironDataMap.insert(qsl("MTTS"), qMakePair(isUserVar, getNewEnvironMTTS()));
    newEnvironDataMap.insert(qsl("TERMINAL_TYPE"), qMakePair(isUserVar, getNewEnvironTerminalType()));

    if (mpHost->mEnableMNES) {
        return newEnvironDataMap;
    }

    // Per https://www.rfc-editor.org/rfc/rfc1572.txt, "USER" and "SYSTEMTYPE" are well-known and will be requested with NEW_ENVIRON_VAR
    //newEnvironDataMap.insert(qsl("USER"), qMakePair(!isUserVar, getNewEnvironValueUser())); // Needs an OPT-IN to be enabled, next PR
    //newEnvironDataMap.insert(qsl("SYSTEMTYPE"), qMakePair(!isUserVar, getNewEnvironValueSystemType())); // Needs an OPT-IN to be enabled, next PR

    // Per https://www.rfc-editor.org/rfc/rfc1572.txt, others will be requested with NEW_ENVIRON_USERVAR
    newEnvironDataMap.insert(qsl("ANSI"), qMakePair(isUserVar, getNewEnvironANSI()));
    newEnvironDataMap.insert(qsl("VT100"), qMakePair(isUserVar, getNewEnvironVT100()));
    newEnvironDataMap.insert(qsl("256_COLORS"), qMakePair(isUserVar, getNewEnviron256Colors()));
    newEnvironDataMap.insert(qsl("UTF-8"), qMakePair(isUserVar, getNewEnvironUTF8()));
    newEnvironDataMap.insert(qsl("OSC_COLOR_PALETTE"), qMakePair(isUserVar, getNewEnvironOSCColorPalette()));
    newEnvironDataMap.insert(qsl("SCREEN_READER"), qMakePair(isUserVar, getNewEnvironScreenReader()));
    newEnvironDataMap.insert(qsl("TRUECOLOR"), qMakePair(isUserVar, getNewEnvironTruecolor()));
    newEnvironDataMap.insert(qsl("TLS"), qMakePair(isUserVar, getNewEnvironTLS()));
    //newEnvironDataMap.insert(qsl("LANGUAGE"), qMakePair(isUserVar, getNewEnvironLanguage())); // Needs an OPT-IN to be enabled, next PR
    newEnvironDataMap.insert(qsl("WORD_WRAP"), qMakePair(isUserVar, getNewEnvironWordWrap()));

    return newEnvironDataMap;
}

// SEND INFO per https://www.rfc-editor.org/rfc/rfc1572
void cTelnet::sendInfoNewEnvironValue(const QString &var)
{
    if (!enableNewEnviron || mpHost->mForceNewEnvironNegotiationOff) {
        return;
    }

    if (mpHost->mEnableMNES && !isMNESVariable(var)) {
        return;
    }

    if (!newEnvironVariablesSent.contains(var)) {
        qDebug() << "We did not update NEW_ENVIRON" << var << "because the server did not request it yet";
        return;
    }

    const QMap<QString, QPair<bool, QString>> newEnvironDataMap = getNewEnvironDataMap();

    if (newEnvironDataMap.contains(var)) {
        qDebug() << "We updated NEW_ENVIRON" << var;

        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = newEnvironDataMap.value(var);
        const bool isUserVar = !mpHost->mEnableMNES && newEnvironData.first;
        const QString val = newEnvironData.second;

        std::string output;
        output += TN_IAC;
        output += TN_SB;
        output += OPT_NEW_ENVIRON;
        output += NEW_ENVIRON_INFO;
        output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(var).toStdString();
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
        }

        output += TN_IAC;
        output += TN_SE;
        socketOutRaw(output);

        if (mpHost->mEnableMNES) {
            if (!val.isEmpty()) {
                qDebug() << "WE inform NEW_ENVIRON (MNES) VAR" << var << "VAL" << val;
            } else {
                qDebug() << "WE inform NEW_ENVIRON (MNES) VAR" << var << "as an empty VAL";
            }
        } else if (!isUserVar) {
            if (!val.isEmpty()) {
                qDebug() << "WE inform NEW_ENVIRON VAR" << var << "VAL" << val;
            } else {
                qDebug() << "WE inform NEW_ENVIRON VAR" << var << "as an empty VAL";
            }
        } else if (!val.isEmpty()) {
            qDebug() << "WE inform NEW_ENVIRON USERVAR" << var << "VAL" << val;
        } else {
            qDebug() << "WE inform NEW_ENVIRON USERVAR" << var << "as an empty VAL";
        }
    }
}

void cTelnet::appendAllNewEnvironValues(std::string &output, const bool isUserVar, const QMap<QString, QPair<bool, QString>> &newEnvironDataMap)
{
    for (auto it = newEnvironDataMap.begin(); it != newEnvironDataMap.end(); ++it) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = it.value();

        if (isUserVar != newEnvironData.first) {
            continue;
        }

        const QString val = newEnvironData.second;

        output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(it.key()).toStdString();
        newEnvironVariablesSent.insert(it.key());
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
        }

        if (!isUserVar) {
            if (!val.isEmpty()) {
                qDebug() << "WE send NEW_ENVIRON VAR" << it.key() << "VAL" << val;
            } else {
                qDebug() << "WE send NEW_ENVIRON VAR" << it.key() << "as an empty VAL";
            }
        } else if (!val.isEmpty()) {
            qDebug() << "WE send NEW_ENVIRON USERVAR" << it.key() << "VAL" << val;
        } else {
            qDebug() << "WE send NEW_ENVIRON USERVAR" << it.key() << "as an empty VAL";
        }
    }
}

void cTelnet::appendNewEnvironValue(std::string &output, const QString &var, const bool isUserVar, const QMap<QString, QPair<bool, QString>> &newEnvironDataMap)
{
    if (newEnvironDataMap.contains(var)) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = newEnvironDataMap.value(var);
        const QString val = newEnvironData.second;

        if (newEnvironData.first != isUserVar) {
            // RFC 1572: If a "type" is not followed by a VALUE (e.g., by another VAR,
            // USERVAR, or IAC SE) then that variable is undefined.
            output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
            output += prepareNewEnvironData(var).toStdString();
            newEnvironVariablesSent.insert(var);

            if (!isUserVar) {
                qDebug() << "WE send NEW_ENVIRON VAR" << var << "with no VAL because we don't maintain it as VAR (use USERVAR!)";
            } else {
                qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "with no VAL because we don't maintain it as USERVAR (use VAR!)";
            }
        } else {
            output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
            output += prepareNewEnvironData(var).toStdString();
            newEnvironVariablesSent.insert(var);
            output += NEW_ENVIRON_VAL;

            // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
            // variable is defined, but has no value.
            if (!val.isEmpty()) {
                output += prepareNewEnvironData(val).toStdString();

                if (!isUserVar) {
                    qDebug() << "WE send NEW_ENVIRON VAR" << var << "VAL" << val;
                } else {
                    qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "VAL" << val;
                }
            } else if (!isUserVar) {
                qDebug() << "WE send NEW_ENVIRON VAR" << var << "as an empty VAL";
            } else {
                qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "as an empty VAL";
            }
        }
    } else {
        // RFC 1572: If a "type" is not followed by a VALUE (e.g., by another VAR,
        // USERVAR, or IAC SE) then that variable is undefined.
        output += isUserVar ? NEW_ENVIRON_USERVAR : NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(var).toStdString();

        if (!isUserVar) {
            qDebug() << "WE send NEW_ENVIRON VAR" << var << "with no VAL because we don't maintain it";
        } else {
            qDebug() << "WE send NEW_ENVIRON USERVAR" << var << "with no VAL because we don't maintain it";
        }
    }
}

// SEND IS per https://www.rfc-editor.org/rfc/rfc1572
void cTelnet::sendIsNewEnvironValues(const QByteArray& payload)
{
    const QMap<QString, QPair<bool, QString>> newEnvironDataMap = getNewEnvironDataMap();

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;

    bool is_uservar = false;
    bool is_var = false;
    QString var;

    for (int i = 0; i < payload.size(); ++i) {
        if (!i && payload.at(i) == NEW_ENVIRON_SEND) {
            continue;
        } else if (!i) {
            return; // Invalid response;
        }

        if (payload.at(i) == NEW_ENVIRON_VAR) {
            if (!var.isEmpty()) {
                appendNewEnvironValue(output, var, (is_uservar ? true : false), newEnvironDataMap);
                var = QString();
            } else if (is_var || is_uservar) {
                appendAllNewEnvironValues(output, (is_uservar ? true : false), newEnvironDataMap);
            }

            is_uservar = false;
            is_var = true;
        } else if (payload.at(i) == NEW_ENVIRON_USERVAR) {
            if (!var.isEmpty()) {
                appendNewEnvironValue(output, var, (is_uservar ? true : false), newEnvironDataMap);
                var = QString();
            } else if (is_var || is_uservar) {
                appendAllNewEnvironValues(output, (is_uservar ? true : false), newEnvironDataMap);
            }

            is_var = false;
            is_uservar = true;
        } else {
            var.append(payload.at(i));
        }
    }

    if (!var.isEmpty()) { // Last on the stack variable
        appendNewEnvironValue(output, var, (is_uservar ? true : false), newEnvironDataMap);
    } else if (is_var || is_uservar) { // Last on the stack VAR or USERVAR with no name
        appendAllNewEnvironValues(output, (is_uservar ? true : false), newEnvironDataMap);
    } else { // No list specified, send the entire list of defined VAR and USERVAR variables
        appendAllNewEnvironValues(output, false, newEnvironDataMap);
        appendAllNewEnvironValues(output, true, newEnvironDataMap);
    }

    output += TN_IAC;
    output += TN_SE;
    socketOutRaw(output);
}

bool cTelnet::isMNESVariable(const QString &var)
{
    static const QStringList validValues = {"CHARSET", "CLIENT_NAME", "CLIENT_VERSION", "MTTS", "TERMINAL_TYPE", "IPADDRESS"};

    return validValues.contains(var);
}

void cTelnet::sendAllMNESValues()
{
    if (!mpHost->mEnableMNES) {
        return;
    }

    const QMap<QString, QPair<bool, QString>> newEnvironDataMap = getNewEnvironDataMap();

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;

    for (auto it = newEnvironDataMap.begin(); it != newEnvironDataMap.end(); ++it) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = it.value();
        const QString val = newEnvironData.second;

        output += NEW_ENVIRON_VAR;
        output += prepareNewEnvironData(it.key()).toStdString();
        newEnvironVariablesSent.insert(it.key());
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << it.key() << "VAL" << val;
        } else {
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << it.key() << "as an empty VAL";
        }
    }

    output += TN_IAC;
    output += TN_SE;
    socketOutRaw(output);
}

void cTelnet::sendMNESValue(const QString &var, const QMap<QString, QPair<bool, QString>> &newEnvironDataMap)
{
    if (!mpHost->mEnableMNES) {
        return;
    }

    if (!isMNESVariable(var)) {
        return;
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_NEW_ENVIRON;
    output += NEW_ENVIRON_IS;
    output += NEW_ENVIRON_VAR;

    if (newEnvironDataMap.contains(var)) {
        // QPair first: NEW_ENVIRON_USERVAR indicator, second: data
        const QPair<bool, QString> newEnvironData = newEnvironDataMap.value(var);
        const QString val = newEnvironData.second;

        output += prepareNewEnvironData(var).toStdString();
        newEnvironVariablesSent.insert(var);
        output += NEW_ENVIRON_VAL;

        // RFC 1572: If a VALUE is immediately followed by a "type" or IAC, then the
        // variable is defined, but has no value.
        if (!val.isEmpty()) {
            output += prepareNewEnvironData(val).toStdString();
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << var << "VAL" << val;
        } else {
            qDebug() << "WE send NEW_ENVIRON (MNES) VAR" << var << "as an empty VAL";
        }
    } else {
        // RFC 1572: If a "type" is not followed by a VALUE (e.g., by another VAR,
        // USERVAR, or IAC SE) then that variable is undefined.
        output += prepareNewEnvironData(var).toStdString();
        output += NEW_ENVIRON_VAL;

        qDebug() << "WE send that we do not maintain NEW_ENVIRON (MNES) VAR" << var;
    }

    output += TN_IAC;
    output += TN_SE;
    socketOutRaw(output);
}

void cTelnet::sendIsMNESValues(const QByteArray& payload)
{
    if (!mpHost->mEnableMNES) {
        return;
    }

    const QMap<QString, QPair<bool, QString>> newEnvironDataMap = getNewEnvironDataMap();

    QString var;

    for (int i = 0; i < payload.size(); ++i) {
        if (!i && payload.at(i) == NEW_ENVIRON_SEND) {
            continue;
        } else if (!i) {
            return; // Invalid response;
        }

        if (payload.at(i) == NEW_ENVIRON_VAR) {
            if (!var.isEmpty()) {
                sendMNESValue(var, newEnvironDataMap);
                var = QString();
            }

            continue;
        }

        var.append(payload.at(i));
    }

    if (!var.isEmpty()) { // Last variable on the stack
        sendMNESValue(var, newEnvironDataMap);
        return;
    }

    sendAllMNESValues(); // No list specified or only a VAR, send the entire list of defined VAR variables
}

void cTelnet::processTelnetCommand(const std::string& telnetCommand)
{
    char ch = telnetCommand[1];
#if defined(DEBUG_TELNET) && (DEBUG_TELNET > 1)
    QString commandType;
    switch (ch) {
    case TN_EOR:
        commandType = QLatin1String("EOR");
        break;
    case TN_SE:
        commandType = QLatin1String("SE");
        break;
    case TN_NOP:
        commandType = QLatin1String("NOP");
        break;
    case TN_DM: // Data Mark
        commandType = QLatin1String("DM");
        break;
    case TN_BRK: // Break
        commandType = QLatin1String("BRK");
        break;
    case TN_IP: // Interupt Process
        commandType = QLatin1String("IP");
        break;
    case TN_AO: // Abort Output
        commandType = QLatin1String("AO");
        break;
    case TN_AYT:
        commandType = QLatin1String("AYT");
        break;
    case TN_EC: // Erase character
        commandType = QLatin1String("EC");
        break;
    case TN_EL: // Erase line
        commandType = QLatin1String("EL");
        break;
    case TN_GA:
        commandType = QLatin1String("GA");
        break;
    case TN_SB:
        commandType = QLatin1String("SB");
        break;
    case TN_WILL:
        commandType = QLatin1String("WILL");
        break;
    case TN_WONT:
        commandType = QLatin1String("WONT");
        break;
    case TN_DO:
        commandType = QLatin1String("DO");
        break;
    case TN_DONT:
        commandType = QLatin1String("DONT");
        break;
    case TN_IAC:
        // Probably won't be seen as it will be stripped off in order for this
        // method to have been called (it'll be in telnetCommand[0])
        commandType = QLatin1String("IAC");
        break;
    default:
        commandType = QString::number((quint8)ch);
    }
    if (telnetCommand.size() > 2) {
        qDebug() << "SERVER sent telnet (" << telnetCommand.size() << " bytes):" << commandType << " + " << decodeOption(telnetCommand[2]);
    } else {
        qDebug() << "SERVER sent telnet (" << telnetCommand.size() << " bytes):" << commandType;
    }
#endif

    char option;
    switch (ch) {
    case TN_GA:
    case TN_EOR: {
        recvdGA = true;
        break;
    }
    case TN_AYT: {
        // This will be unaffected by the Mud Server encoding setting:
        std::string output = "YES";
        socketOutRaw(output);
        break;
    }
    case TN_WILL: {
        //server wants to enable some option (or he sends a timing-mark)...
        option = telnetCommand[2];
        const auto idxOption = static_cast<size_t>(option);
#ifdef DEBUG_TELNET
        qDebug().nospace().noquote() << "Server sent telnet IAC WILL " << decodeOption(option);
#endif

        if (option == OPT_EOR) {
            //EOR support (END OF RECORD=TN_GA)
            qDebug() << "EOR enabled";
            sendTelnetOption(TN_DO, OPT_EOR);
            break;
        }

        if (option == OPT_NEW_ENVIRON) {
            // NEW_ENVIRON support per https://www.rfc-editor.org/rfc/rfc1572.txt
            if (mpHost->mForceNewEnvironNegotiationOff) { // We DONT welcome the WILL
                sendTelnetOption(TN_DONT, option);

                if (enableNewEnviron) {
                    raiseProtocolEvent("sysProtocolDisabled", "NEW_ENVIRON");
                }

                qDebug() << "Rejecting NEW_ENVIRON, because Force NEW_ENVIRON negotiation off is checked.";
            } else {
                sendTelnetOption(TN_DO, OPT_NEW_ENVIRON);
                enableNewEnviron = true; // We negotiated, the game server is welcome to SEND now
                raiseProtocolEvent("sysProtocolEnabled", "NEW_ENVIRON");
                qDebug() << "NEW_ENVIRON enabled";
            }

            break;
        }

        if (option == OPT_CHARSET) {
            // CHARSET support per https://tools.ietf.org/html/rfc2066
            if (mpHost->mFORCE_CHARSET_NEGOTIATION_OFF) { // We DONT welcome the WILL
                sendTelnetOption(TN_DONT, option);

                if (enableCHARSET) {
                    raiseProtocolEvent("sysProtocolDisabled", "CHARSET");
                }

                enableCHARSET = false;
                qDebug() << "Rejecting CHARSET, because Force CHARSET negotiation off is checked.";
            } else {
                sendTelnetOption(TN_DO, OPT_CHARSET);
                enableCHARSET = true; // We negotiated, the game server is welcome to REQUEST now
                qDebug() << "CHARSET enabled";
                raiseProtocolEvent("sysProtocolEnabled", "CHARSET");
            }

            break;
        }

        if (option == OPT_MSDP) {
            //MSDP support
            if (!mpHost->mEnableMSDP) {
                sendTelnetOption(TN_DONT, OPT_MSDP);

                if (enableMSDP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MSDP");
                }

                enableMSDP = false;
                break;
            } else {
                std::string output;

                enableMSDP = true;
                sendTelnetOption(TN_DO, OPT_MSDP);
                //need to send MSDP start sequence: IAC   SB MSDP MSDP_VAR "LIST" MSDP_VAL "COMMANDS" IAC SE
                //NOTE: MSDP does not need quotes for string/vals
                output += TN_IAC;
                output += TN_SB;
                output += OPT_MSDP;
                output += MSDP_VAR;
                output += "LIST";
                output += MSDP_VAL;
                output += "COMMANDS";
                output += TN_IAC;
                output += TN_SE;
                // This will be unaffected by Mud Server encoding:
                socketOutRaw(output);

                // send client configurable variables e.g.
                // IAC SB MSDP MSDP_VAR "CLIENT" MSDP_VAL "Mudlet" MSDP_VAR "VERSION" MSDP_VAL "4.19" IAC SE
                output = TN_IAC;
                output += TN_SB;
                output += OPT_MSDP;
                output += MSDP_VAR;
                output += "CLIENT";
                output += MSDP_VAL;
                output += "Mudlet";
                output += MSDP_VAR;
                output += "VERSION";
                output += MSDP_VAL;
                output += encodeAndCookBytes(std::string(APP_VERSION) + mudlet::self()->mAppBuild.toUtf8().constData());
                output += TN_IAC;
                output += TN_SE;
                socketOutRaw(output);
#ifdef DEBUG_TELNET
                qDebug() << "WE send telnet IAC DO MSDP";
#endif
                raiseProtocolEvent("sysProtocolEnabled", "MSDP");
                break;
            }
        }

        if (option == OPT_ATCP) {
            // ATCP support
            if (mpHost->mEnableGMCP) {
                sendTelnetOption(TN_DONT, OPT_ATCP);

                if (enableATCP) {
                    raiseProtocolEvent("sysProtocolDisabled", "ATCP");
                }

                enableATCP = false;
                break;
            }

            qDebug() << "ATCP enabled";
            enableATCP = true;
            sendTelnetOption(TN_DO, OPT_ATCP);

            std::string output;
            output += TN_IAC;
            output += TN_SB;
            output += OPT_ATCP;
            std::string atcpOptions = std::string("hello Mudlet ") + std::string(APP_VERSION) + mudlet::self()->mAppBuild.toUtf8().constData() + "\ncomposer 1\nchar_vitals 1\nroom_brief 1\nroom_exits 1\nmap_display 1\n";
            output += encodeAndCookBytes(atcpOptions);
            output += TN_IAC;
            output += TN_SE;
            socketOutRaw(output);

            raiseProtocolEvent("sysProtocolEnabled", "ATCP");
            break;
        }

        if (option == OPT_GMCP) {
            if (!mpHost->mEnableGMCP) {
                sendTelnetOption(TN_DONT, OPT_GMCP);

                if (enableGMCP) {
                    raiseProtocolEvent("sysProtocolDisabled", "GMCP");
                }

                enableGMCP = false;
                break;
            }

            enableGMCP = true;
            sendTelnetOption(TN_DO, OPT_GMCP);
            qDebug() << "GMCP enabled";

            std::string output;
            output = TN_IAC;
            output += TN_SB;
            output += OPT_GMCP;
            // mudlet::self()->mAppBuild could, conceivably contain a non-ASCII character:
            output += encodeAndCookBytes(std::string(R"(Core.Hello { "client": "Mudlet", "version": ")") + APP_VERSION + mudlet::self()->mAppBuild.toUtf8().constData() + std::string(R"("})"));
            output += TN_IAC;
            output += TN_SE;
            socketOutRaw(output);

            output = TN_IAC;
            output += TN_SB;
            output += OPT_GMCP;
            output += R"(Core.Supports.Set [ "Char 1", "Char.Skills 1", "Char.Items 1", "Room 1", "IRE.Rift 1", "IRE.Composer 1", "External.Discord 1", "Client.Media 1", "Char.Login 1"])";
            output += TN_IAC;
            output += TN_SE;
            socketOutRaw(output);

            if (mudlet::self()->mDiscord.libraryLoaded()) {
                output = TN_IAC;
                output += TN_SB;
                output += OPT_GMCP;
                output += "External.Discord.Hello";
                output += TN_IAC;
                output += TN_SE;

                socketOutRaw(output);
            }

            raiseProtocolEvent("sysProtocolEnabled", "GMCP");
            break;
        }

        if (option == OPT_MSSP) {
            if (!mpHost->mEnableMSSP) {
                sendTelnetOption(TN_DONT, OPT_MSSP);

                if (enableMSSP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MSSP");
                }

                enableMSSP = false;
                break;
            }

            enableMSSP = true;
            sendTelnetOption(TN_DO, OPT_MSSP);
            qDebug() << "MSSP enabled";
            raiseProtocolEvent("sysProtocolEnabled", "MSSP");
            break;
        }

        if (option == OPT_MSP) {
            if (!mpHost->mEnableMSP) {
                sendTelnetOption(TN_DONT, OPT_MSP);

                if (enableMSP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MSP");
                }

                enableMSP = false;
                break;
            }

            enableMSP = true;
            sendTelnetOption(TN_DO, OPT_MSP);
            qDebug() << "MSP enabled";
            raiseProtocolEvent("sysProtocolEnabled", "MSP");
            break;
        }

        if (option == OPT_MXP) {
            if (!mpHost->mEnableMXP) {
                sendTelnetOption(TN_DONT, OPT_MXP);
                mpHost->mMxpProcessor.disable();

                if (enableMXP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MXP");
                }

                enableMXP = false;
                break;
            }

            enableMXP = true;
            sendTelnetOption(TN_DO, OPT_MXP);
            mpHost->mMxpProcessor.enable();

            qDebug() << "MXP enabled";
            raiseProtocolEvent("sysProtocolEnabled", "MXP");
            break;
        }

        if (option == OPT_102) {
            // Aardwulf channel 102 support
            qDebug() << "Aardwulf channel 102 support enabled";
            enableChannel102 = true;
            sendTelnetOption(TN_DO, OPT_102);
            raiseProtocolEvent("sysProtocolEnabled", "channel102");
            break;
        }

        heAnnouncedState[idxOption] = true;
        if (triedToEnable[idxOption]) {
            hisOptionState[idxOption] = true;
            triedToEnable[idxOption] = false;
        } else {
            if (!hisOptionState[idxOption]) {
                //only if this is not set; if it's set, something's wrong with the server
                //(according to telnet specification, option announcement may not be
                //unless explicitly requested)

                if (option == OPT_ECHO) {
                    sendTelnetOption(TN_DO, option);
                    hisOptionState[idxOption] = true;
                    mpHost->setRemoteEchoingActive(true);
                    qDebug() << "Enabling Server ECHOing of our output - perhaps he want us to type a password?";
                } else if ((option == OPT_STATUS) || (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS)) {
                    sendTelnetOption(TN_DO, option);
                    hisOptionState[idxOption] = true;
                } else if ((option == OPT_COMPRESS) || (option == OPT_COMPRESS2)) {
                    //these are handled separately, as they're a bit special
                    if (mpHost->mFORCE_NO_COMPRESSION) {
                        sendTelnetOption(TN_DONT, option);
                        hisOptionState[idxOption] = false;
                        qDebug().nospace().noquote() << "Rejecting MCCP v" << (option == OPT_COMPRESS ? "1" : "2") << ", because the 'Force compression off' option is enabled.";
                    } else if ((option == OPT_COMPRESS) && (hisOptionState[static_cast<int>(OPT_COMPRESS2)])) {
                        //protocol says: reject MCCP v1 if you have previously accepted MCCP v2...
                        sendTelnetOption(TN_DONT, option);
                        hisOptionState[idxOption] = false;
                        qDebug() << "Rejecting MCCP v1, because v2 has already been negotiated.";
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
        //server refuses to enable some option
        option = telnetCommand[2];
        const auto idxOption = static_cast<size_t>(option);
#ifdef DEBUG_TELNET
        qDebug().nospace().noquote() << "Server sent telnet IAC WONT " << decodeOption(option);
#endif
        if (triedToEnable[idxOption]) {
            hisOptionState[idxOption] = false;
            triedToEnable[idxOption] = false;
            heAnnouncedState[idxOption] = true;
        } else {
            if (option == OPT_NEW_ENVIRON) {
                // NEW_ENVIRON got turned off
                enableNewEnviron = false;
                raiseProtocolEvent("sysProtocolDisabled", "NEW_ENVIRON");
            }

            if (option == OPT_CHARSET) {
                // CHARSET got turned off per https://tools.ietf.org/html/rfc2066
                enableCHARSET = false;
                raiseProtocolEvent("sysProtocolDisabled", "CHARSET");
            }

            if (option == OPT_MSDP) {
                // MSDP got turned off
                enableMSDP = false;
                raiseProtocolEvent("sysProtocolDisabled", "MSDP");
            }

            if (option == OPT_ATCP) {
                // ATCP got turned off
                enableATCP = false;
                raiseProtocolEvent("sysProtocolDisabled", "ATCP");
            }

            if (option == OPT_GMCP) {
                // GMCP got turned off
                enableGMCP = false;
                raiseProtocolEvent("sysProtocolDisabled", "GMCP");
            }

            if (option == OPT_MSSP) {
                // MSSP got turned off
                enableMSSP = false;
                raiseProtocolEvent("sysProtocolDisabled", "MSSP");
            }

            if (option == OPT_MSP) {
                // MSP got turned off
                enableMSP = false;
                raiseProtocolEvent("sysProtocolDisabled", "MSP");
            }

            if (option == OPT_MXP) {
                // MXP got turned off
                enableMXP = false;
                mpHost->mMxpProcessor.disable();
                raiseProtocolEvent("sysProtocolDisabled", "MXP");
            }

            if (option == OPT_102) {
                // channel 102 support
                enableChannel102 = false;
                raiseProtocolEvent("sysProtocolDisabled", "channel102");
            }

            //send DONT if needed (see RFC 854 for details)
            if (hisOptionState[idxOption] || (heAnnouncedState[idxOption])) {
                sendTelnetOption(TN_DONT, option);
                hisOptionState[idxOption] = false;

                if (option == OPT_ECHO) {
                    mpHost->setRemoteEchoingActive(false);
                    qDebug() << "Server is stopping the ECHOing our output - so back to normal after, perhaps, sending a password...";
                }

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
        //server wants us to enable some option
        option = telnetCommand[2];
        const auto idxOption = static_cast<size_t>(option);
#ifdef DEBUG_TELNET
        qDebug().nospace().noquote() << "Server sent telnet IAC DO " << decodeOption(option);
#endif

        if (option == OPT_NEW_ENVIRON) {
            // NEW_ENVIRON support per https://www.rfc-editor.org/rfc/rfc1572.txt
            if (mpHost->mForceNewEnvironNegotiationOff) { // We WONT welcome the DO
                sendTelnetOption(TN_WONT, option);

                if (enableNewEnviron) {
                    raiseProtocolEvent("sysProtocolDisabled", "NEW_ENVIRON");
                }

                qDebug() << "Rejecting NEW_ENVIRON, because Force NEW_ENVIRON negotiation off is checked.";
            } else { // We have already negotiated the use of the option by us (We WILL welcome the DO)
                sendTelnetOption(TN_WILL, OPT_NEW_ENVIRON);
                enableNewEnviron = true; // We negotiated, the game server is welcome to SEND now
                raiseProtocolEvent("sysProtocolEnabled", "NEW_ENVIRON");
                qDebug() << "NEW_ENVIRON enabled";
            }

            break;
        }

        if (option == OPT_CHARSET) {
            // CHARSET support per https://tools.ietf.org/html/rfc2066
            if (mpHost->mFORCE_CHARSET_NEGOTIATION_OFF) { // We WONT welcome the DO
                sendTelnetOption(TN_WONT, option);

                if (enableCHARSET) {
                    raiseProtocolEvent("sysProtocolDisabled", "CHARSET");
                }

                enableCHARSET = false;
                qDebug() << "Rejecting CHARSET, because Force CHARSET negotiation off is checked.";
            } else  { // We have already negotiated the use of the option by us (We WILL welcome the DO)
                sendTelnetOption(TN_WILL, OPT_CHARSET);
                enableCHARSET = true; // We negotiated, the game server is welcome to REQUEST now
                qDebug() << "CHARSET enabled";
                raiseProtocolEvent("sysProtocolEnabled", "CHARSET");
            }

            break;
        }

        if (option == OPT_MSDP) {
            if (mpHost->mEnableMSDP) {
                enableMSDP = true;
                sendTelnetOption(TN_WILL, OPT_MSDP);
                raiseProtocolEvent("sysProtocolEnabled", "MSDP");
            } else {
                sendTelnetOption(TN_WONT, OPT_MSDP);

                if (enableMSDP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MSDP");
                }

                enableMSDP = false;
            }
            break;
        }

        if (option == OPT_ATCP) {
            if (!mpHost->mEnableGMCP) {
                enableATCP = true;
                sendTelnetOption(TN_WILL, OPT_ATCP);
                raiseProtocolEvent("sysProtocolEnabled", "ATCP");
            } else {
                sendTelnetOption(TN_WONT, OPT_ATCP);

                if (enableATCP) {
                    raiseProtocolEvent("sysProtocolDisabled", "ATCP");
                }

                enableATCP = false;
            }
            break;
        }

        if (option == OPT_GMCP) {
            if (mpHost->mEnableGMCP) {
                enableGMCP = true;
                sendTelnetOption(TN_WILL, OPT_GMCP);
                raiseProtocolEvent("sysProtocolEnabled", "GMCP");
            } else {
                sendTelnetOption(TN_WONT, OPT_GMCP);

                if (enableGMCP) {
                    raiseProtocolEvent("sysProtocolDisabled", "GMCP");
                }

                enableGMCP = false;
            }
            break;
        }

        if (option == OPT_MSSP) {
            if (mpHost->mEnableMSSP) {
                enableMSSP = true;
                sendTelnetOption(TN_WILL, OPT_MSSP);
                raiseProtocolEvent("sysProtocolEnabled", "MSSP");
            } else {
                sendTelnetOption(TN_WONT, OPT_MSSP);

                if (enableMSSP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MSSP");
                }

                enableMSSP = false;
            }
            break;
        }

        if (option == OPT_MSP) {
            if (mpHost->mEnableMSP) {
                enableMSP = true;
                sendTelnetOption(TN_WILL, OPT_MSP);
                raiseProtocolEvent("sysProtocolEnabled", "MSP");
            } else {
                sendTelnetOption(TN_WONT, OPT_MSP);

                if (enableMSP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MSP");
                }

                enableMSP = false;
            }
            break;
        }

        if (option == OPT_MXP) {
            if (mpHost->mEnableMXP) {
                enableMXP = true;
                sendTelnetOption(TN_WILL, OPT_MXP);
                mpHost->mMxpProcessor.enable();
                raiseProtocolEvent("sysProtocolEnabled", "MXP");
            } else {
                sendTelnetOption(TN_WONT, OPT_MXP);
                mpHost->mMxpProcessor.disable();

                if (enableMXP) {
                    raiseProtocolEvent("sysProtocolDisabled", "MXP");
                }

                enableMXP = false;
            }
            break;
        }

        if (option == OPT_102) {
            // channel 102 support
            enableChannel102 = true;
            sendTelnetOption(TN_WILL, OPT_102);
            raiseProtocolEvent("sysProtocolEnabled", "channel102");
            break;
        }

        if (option == OPT_TIMING_MARK) {
            // See https://www.rfc-editor.org/rfc/rfc860.txt
            qDebug() << "We have received a DO TIMING_MARK request, sending a WONT as we do not actually do anything with it but even that can be useful to the sender.";
            sendTelnetOption(TN_WONT, option);
        } else if (!myOptionState[idxOption]) {
            // only if the option is currently disabled

            if ((option == OPT_STATUS) || (option == OPT_NAWS) || (option == OPT_TERMINAL_TYPE)) {
                if (option == OPT_STATUS) {
                    qDebug() << "We ARE willing to enable telnet option STATUS";
                }
                if (option == OPT_TERMINAL_TYPE) {
                    qDebug() << "We ARE willing to enable telnet option TERMINAL_TYPE";
                }
                if (option == OPT_NAWS) {
                    qDebug() << "We ARE willing to enable telnet option NAWS";
                }
                sendTelnetOption(TN_WILL, option);
                myOptionState[idxOption] = true;
                announcedState[idxOption] = true;
            } else {
                qDebug() << "We are NOT WILLING to enable this telnet option.";
                sendTelnetOption(TN_WONT, option);
                myOptionState[idxOption] = false;
                announcedState[idxOption] = true;
            }
        }
        if (option == OPT_NAWS) {
            //NAWS
            // Ensure that the stored copies of the screen dimensions have been
            // reset before we do this so that they are different from real,
            // used values:
            mNaws_x = 0;
            mNaws_y = 0;
            // thus sending of the values is performed when we check them:
            checkNAWS();
        }
        break;
    }
    case TN_DONT: {
        //only respond if value changed or if this option has not been announced yet
        option = telnetCommand[2];
        const auto idxOption = static_cast<size_t>(option);
#ifdef DEBUG_TELNET
        qDebug().nospace().noquote() << "Server sent telnet IAC DONT " << decodeOption(option);
#endif

        if (option == OPT_NEW_ENVIRON) {
            // NEW_ENVIRON got turned off
            enableNewEnviron = false;
            raiseProtocolEvent("sysProtocolDisabled", "NEW_ENVIRON");
        }

        if (option == OPT_CHARSET) {
            // CHARSET got turned off per https://tools.ietf.org/html/rfc2066
            enableCHARSET = false;
            raiseProtocolEvent("sysProtocolDisabled", "CHARSET");
        }

        if (option == OPT_MSDP) {
            // MSDP got turned off
            enableMSDP = false;
            raiseProtocolEvent("sysProtocolDisabled", "MSDP");
        }

        if (option == OPT_ATCP) {
            // ATCP got turned off
            enableATCP = false;
            raiseProtocolEvent("sysProtocolDisabled", "ATCP");
        }

        if (option == OPT_GMCP) {
            // GMCP got turned off
            enableGMCP = false;
            raiseProtocolEvent("sysProtocolDisabled", "GMCP");
        }

        if (option == OPT_MSSP) {
            // MSSP got turned off
            enableMSSP = false;
            raiseProtocolEvent("sysProtocolDisabled", "MSSP");
        }

        if (option == OPT_MSP) {
            // MSP got turned off
            enableMSP = false;
            raiseProtocolEvent("sysProtocolDisabled", "MSP");
        }

        if (option == OPT_MXP) {
            // MXP got turned off
            enableMXP = false;
            mpHost->mMxpProcessor.disable();
            raiseProtocolEvent("sysProtocolDisabled", "MXP");
        }

        if (option == OPT_102) {
            // channel 102 support
            enableChannel102 = false;
            raiseProtocolEvent("sysProtocolDisabled", "channel102");
        }

        if (myOptionState[idxOption] || (!announcedState[idxOption])) {
            sendTelnetOption(TN_WONT, option);
            announcedState[idxOption] = true;
        }
        myOptionState[idxOption] = false;
        break;
    }

    case TN_SB: {
        option = telnetCommand[2];

        // NEW_ENVIRON
        if (option == OPT_NEW_ENVIRON && enableNewEnviron) {
            QByteArray payload = QByteArray::fromRawData(telnetCommand.c_str(), telnetCommand.size());

            if (telnetCommand.size() < 6) {
                return; // Invalid NEW_ENVIRON syntax
            }

            // Trim off the Telnet suboption bytes from beginning (3) and end (2)
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);

            if (mpHost->mEnableMNES) {
                sendIsMNESValues(payload);
                return;
            }

            sendIsNewEnvironValues(payload);
            return;
        }

        // CHARSET
        if (option == OPT_CHARSET && enableCHARSET) {
            QByteArray payload = telnetCommand.c_str();

            if (payload.size() < 6) {
                return;
            }

            // Trim off the Telnet suboption bytes from beginning (3) and end (2)
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);

            // CHARSET support per https://tools.ietf.org/html/rfc2066
            if (telnetCommand[3] == CHARSET_REQUEST) {
                if (payload.startsWith("[TTABLE]1")) { // No translate table support.  Discard.
                    payload.remove(0, 9);
                }

                auto characterSetList = payload.split(payload[1]); // Second character is the separator.
                QByteArray acceptedCharacterSet;

                if (!characterSetList.isEmpty()) {
                    for (int i = 1; i < characterSetList.size(); ++i) {
                        QByteArray characterSet = characterSetList.at(i).toUpper();

                        if (mAcceptableEncodings.contains(characterSet) ||
                            mAcceptableEncodings.contains(("M_" + characterSet)) ||
                            characterSet.contains(QByteArray("ASCII"))) { // Accept variants of ASCII
                            acceptedCharacterSet = characterSet;
                            break;
                        }

                        if (characterSet.startsWith("ISO-") &&  // Accept "ISO-####-#" variant of "ISO ####-#"
                            mAcceptableEncodings.contains(QByteArray("ISO " + characterSet.mid(4)))) {
                            acceptedCharacterSet = characterSet;
                            break;
                        }

                        if (!characterSet.startsWith("ISO ") &&
                            characterSet.startsWith("ISO") &&  // Accept "ISO####-#" variant of "ISO ####-#"
                            mAcceptableEncodings.contains(QByteArray("ISO " + characterSet.mid(3)))) {
                            acceptedCharacterSet = characterSet;
                            break;
                        }
                    }
                }

                std::string output;
                output += TN_IAC;
                output += TN_SB;
                output += OPT_CHARSET;

                if (!acceptedCharacterSet.isEmpty()) {
                    QByteArray value;
                    if (acceptedCharacterSet.contains(QByteArray("ASCII"))) {
                        value = QByteArray("ASCII");
                        setEncoding(value, true); // Force variants of ASCII to ASCII
                    } else if (acceptedCharacterSet.startsWith("ISO-")) {
                        value = QByteArray("ISO " + acceptedCharacterSet.mid(4));
                        setEncoding(value, true); // Align with TEncodingTable::csmEncodings
                    } else if (acceptedCharacterSet.startsWith("ISO") && !acceptedCharacterSet.startsWith("ISO ")) {
                        value = QByteArray("ISO " + acceptedCharacterSet.mid(3));
                        setEncoding(value, true); // Align with TEncodingTable::csmEncodings
                    } else {
                        value = acceptedCharacterSet;
                        setEncoding(value, true);
                    }
                    qDebug() << "Game changed encoding to" << value;

                    output += CHARSET_ACCEPTED;
                    output += encodeAndCookBytes(acceptedCharacterSet.toStdString());
                } else {
                    output += CHARSET_REJECTED;
                }

                output += TN_IAC;
                output += TN_SE;
                socketOutRaw(output);
            } else if (telnetCommand[3] == CHARSET_ACCEPTED) {
                // Case unlikely.  Mudlet does not initiate negotiations yet.  Do nothing.
            } else if (telnetCommand[3] == CHARSET_REJECTED) {
                // Case unlikely.  Mudlet does not initiate negotiations yet.  Do nothing.
            } else if (telnetCommand[3] == CHARSET_TTABLE_IS) {
                // Mudlet does not support translate tables
                // Required to respond per the specification
                std::string output;
                output += TN_IAC;
                output += TN_SB;
                output += OPT_CHARSET;
                output += CHARSET_TTABLE_REJECTED;
                output += TN_IAC;
                output += TN_SE;
                socketOutRaw(output);
            }

            return;
        }

        // MSDP
        if (option == OPT_MSDP) {
            // Using a QByteArray means there is no consideration of encoding
            // used - it is just bytes...
            QByteArray rawData = telnetCommand.c_str();

            if (telnetCommand.size() < 6) {
                return;
            }

            rawData = rawData.replace(TN_BELL, QByteArray("\\\\007"));

            rawData = rawData.replace("\x1b", QByteArray("\\\\027"));

            // rawData is in the Mud Server's encoding, trim off the Telnet suboption
            // bytes from beginning (3) and end (2):
            rawData = rawData.mid(3, static_cast<int>(rawData.size()) - 5);
            mpHost->mLuaInterpreter.msdp2Lua(rawData.constData());
            return;
        }

        // ATCP
        if (option == OPT_ATCP) {
            QByteArray payload = telnetCommand.c_str();
            if (payload.size() < 6) {
                return;
            }
            // payload is in the Mud Server's encoding, trim off the Telnet
            // suboption bytes from beginning (3) and end (2):
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);
            setATCPVariables(payload);

            if (payload.startsWith(QByteArray("Auth.Request"))) {
                std::string output;
                output += TN_IAC;
                output += TN_SB;
                output += OPT_ATCP;
                // mudlet::self()->mAppBuild *could* be a non-ASCII UTF-8 string:
                std::string atcpOptions = std::string("hello Mudlet ") + std::string(APP_VERSION) + mudlet::self()->mAppBuild.toUtf8().constData() + "\ncomposer 1\nchar_vitals 1\nroom_brief 1\nroom_exits 1\nmap_display 1\n";
                output += encodeAndCookBytes(atcpOptions);
                output += TN_IAC;
                output += TN_SE;
                socketOutRaw(output);
            } else if (payload.toLower().startsWith(QByteArray("client.gui"))) {
                if (!mpHost->mAcceptServerGUI) {
                    return;
                }

                // payload is still in MUD server encoding at this point, this
                // will not be a problem for the previous string tests as those
                // both use ASCII characters that will not change if the
                // encoding is wrong.
                QString msg = decodeBytes(payload);
                QString version = msg.section(QChar::LineFeed, 0);
                version.remove(qsl("Client.GUI "), Qt::CaseInsensitive);
                version.replace(QChar::LineFeed, QChar::Space);
                version = version.section(QChar::Space, 0, 0);

                QString url = msg.section(QChar::LineFeed, 1);
                QString packageName = url.section(QLatin1Char('/'), -1);
                QString fileName = packageName;
                // As this is a file name it must be handled case insensitively to allow
                // for platforms which may not be case sensitive (MacOs!):
                packageName.remove(qsl(".zip"), Qt::CaseInsensitive);
                packageName.remove(qsl(".trigger"), Qt::CaseInsensitive);
                packageName.remove(qsl(".xml"), Qt::CaseInsensitive);
                packageName.remove(qsl(".mpackage"), Qt::CaseInsensitive);
                packageName.remove(QLatin1Char('/'));
                packageName.remove(QLatin1Char('\\'));
                packageName.remove(QLatin1Char('.'));

                // Check if the package is installed
                if (!mpHost->mInstalledPackages.contains(packageName)) {
                    // Package is not installed, initiate the download
                    mpHost->mServerGUI_Package_version = version;
                    downloadAndInstallGUIPackage(packageName, fileName, url);
                } else if (mpHost->mServerGUI_Package_version != version) {
                    // Check if the version is different and handle the upgrade
                    postMessage(tr("[ INFO ]  - Upgrading the GUI to new version '%1' from version '%2' (url='%3').")
                                .arg(version, mpHost->mServerGUI_Package_version, url));

                    // Uninstall the old version
                    mpHost->uninstallPackage(mpHost->mServerGUI_Package_name != qsl("nothing") ? mpHost->mServerGUI_Package_name : packageName, enums::PackageModuleType::Package);

                    // Download and install the new version
                    mpHost->mServerGUI_Package_version = version;
                    downloadAndInstallGUIPackage(packageName, fileName, url);
                }
            }
            return;
        }

        // Original fix by CR, second revision by MH - To take out normal MCCP version 1 option and 2, no need for this. -MH
        // TODO: Remove these comments. Old boolean taken out for MCCP, and other options which were un-needed code. Rev.3 -MH //

        // GMCP
        if (option == OPT_GMCP) {
            QByteArray payload = telnetCommand.c_str();
            if (payload.size() < 6) {
                return;
            }
            // payload is in the Mud Server's encoding, trim off the Telnet suboption
            // bytes from beginning (3) and end (2):
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);

            // strip first 3 characters to get rid of <IAC><SB><201>
            // and strip the last 2 characters to get rid of <IAC><TN_SE>
            setGMCPVariables(payload);
            return;
        }

        // MSSP
        if (option == OPT_MSSP) {
            QByteArray payload = telnetCommand.c_str();
            if (payload.size() < 6) {
                return;
            }
            // payload is in the Mud Server's encoding, trim off the Telnet suboption
            // bytes from beginning (3) and end (2):
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);

            // strip first 3 characters to get rid of <IAC><SB><70>
            // and strip the last 2 characters to get rid of <IAC><TN_SE>
            setMSSPVariables(payload);
            return;
        }

        // MSP
        if (option == OPT_MSP) {
            QByteArray payload = telnetCommand.c_str();
            if (payload.size() < 6) {
                return;
            }
            // payload is in the Mud Server's encoding, trim off the Telnet suboption
            // bytes from beginning (3) and end (2):
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);

            // strip first 3 characters to get rid of <IAC><SB><90>
            // and strip the last 2 characters to get rid of <IAC><TN_SE>
            setMSPVariables(payload);
            return;
        }

        if (option == OPT_102) {
            QByteArray payload = telnetCommand.c_str();
            if (payload.size() < 6) {
                return;
            }
            // payload is in the Mud Server's encoding, trim off the Telnet suboption
            // bytes from beginning (3) and end (2):
            payload = payload.mid(3, static_cast<int>(payload.size()) - 5);

            setChannel102Variables(payload);
            return;
        }

        switch (option) { //switch 2
        case OPT_STATUS: {
            if (telnetCommand.length() >= 6 && telnetCommand[3] == TNSB_SEND && telnetCommand[4] == TN_IAC && telnetCommand[5] == TN_SE) {
                //request to send all enabled commands; if server sends his
                //own list of commands, we just ignore it (well, he shouldn't
                //send anything, as we do not request anything, but there are
                //so many servers out there, that you can never be sure...)
                // FIXME: This is damaged at the moment as we do not properly take care of the bits for the protocols that we manage ourselves e.g. ATCP/GMCP/MSDP/MXP etc...
                std::string cmd;
                cmd += TN_IAC;
                cmd += TN_SB;
                cmd += OPT_STATUS;
                cmd += TNSB_IS;
                for (size_t i = 0; i < 256; ++i) {
                    if (myOptionState[i]) {
                        cmd += TN_WILL;
                        cmd += i;
                        if (i == static_cast<unsigned char>(TN_SE)) {
                            // Handle corner case where sub-option value is the same as TN_SE (240)
                            cmd += i;
                        }
                    }
                    if (hisOptionState[i]) {
                        cmd += TN_DO;
                        cmd += i;
                        if (i == static_cast<unsigned char>(TN_SE)) {
                            // Handle corner case where byte value is TN_SE
                            cmd += i;
                        }
                    }
                }
                cmd += TN_IAC;
                cmd += TN_SE;
                // This works as handling the status is exempt from the need to
                // escape values that would themselves be interpreted as Telnet
                // protocol bytes themselves - except for the corner case when
                // the sub-option is 240 as described in: RFC 859
                // https://tools.ietf.org/html/rfc859 :
                socketOutRaw(cmd);
            }
            break;
        }

        case OPT_TERMINAL_TYPE: {
            if (telnetCommand.length() >= 6 && telnetCommand[3] == TNSB_SEND && telnetCommand[4] == TN_IAC && telnetCommand[5] == TN_SE) {
                if (myOptionState[static_cast<size_t>(OPT_TERMINAL_TYPE)]) {
                    std::string cmd;
                    cmd += TN_IAC;
                    cmd += TN_SB;
                    cmd += OPT_TERMINAL_TYPE;
                    cmd += TNSB_IS;

                    switch (mCycleCountMTTS) {
                        case 0: {
                            const QString clientName = getNewEnvironClientName();
                            cmd += clientName.toStdString();

                            if (mpHost->mEnableMTTS) { // If we don't MTTS, remainder of the cases do not execute.
                                mCycleCountMTTS++;
                                qDebug() << "MTTS enabled";
                                qDebug() << "WE send TERMINAL_TYPE (MTTS) terminal type is" << clientName;
                            } else {
                                qDebug() << "WE send TERMINAL_TYPE is" << clientName;
                            }

                            break;
                        }

                        case 1: {
                            const QString mttsTerminalType = getNewEnvironTerminalType();
                            cmd += mttsTerminalType.toStdString(); // Example: ANSI-TRUECOLOR
                            mCycleCountMTTS++;
                            qDebug() << "WE send TERMINAL_TYPE (MTTS) terminal type is" << mttsTerminalType;
                            break;
                        }

                        default: {
                            const QString mttsTerminalStandards = getNewEnvironMTTS();
                            cmd += qsl("MTTS %1").arg(mttsTerminalStandards).toStdString(); // Example: MTTS 2349

                            if (mCycleCountMTTS == 2) {
                                mCycleCountMTTS++;
                                qDebug() << "WE send TERMINAL_TYPE (MTTS) bitvector is" << mttsTerminalStandards;
                            } else {
                                mCycleCountMTTS = 0; // Send the bitvector twice, then reset (0) to finish MTTS negotiation
                                qDebug() << "WE send TERMINAL_TYPE (MTTS) bitvector is" << mttsTerminalStandards << "(repeated)";
                            }
                        }
                    }

                    cmd += TN_IAC;
                    cmd += TN_SE;
                    socketOutRaw(cmd);
                }
            }
        }
        //other cmds should not arrive, as they were not negotiated.
        //if they do, they are merely ignored
        } //end switch 2
        //other commands are simply ignored (NOP and such, see .h file for list)
    }
    } //end switch 1

    // raise sysTelnetEvent for all unhandled protocols
    // EXCEPT TN_GA / TN_EOR, which come at the end of every transmission, for performance reasons
    if (telnetCommand[1] != TN_GA && telnetCommand[1] != TN_EOR) {
        auto type = static_cast<unsigned char>(telnetCommand[1]);
        auto telnetOption = static_cast<unsigned char>(telnetCommand[2]);
        QString msg = telnetCommand.c_str();
        if (telnetCommand.size() >= 6) {
            msg = msg.mid(3, telnetCommand.size() - 5);
        }

        TEvent event {};
        event.mArgumentList.append(qsl("sysTelnetEvent"));
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

// msg is in the Mud Server encoding
void cTelnet::setATCPVariables(const QByteArray& msg)
{
    QString transcodedMsg;
    if (mpOutOfBandDataIncomingCodec) {
        // Message is encoded
        transcodedMsg = mpOutOfBandDataIncomingCodec->toUnicode(msg);
    } else {
        // Message is in ASCII (though this can handle Utf-8):
        transcodedMsg = msg;
    }

    QString var;
    QString arg;
    bool single = true;
    if (transcodedMsg.indexOf('\n') > -1) {
        var = transcodedMsg.section(QChar::LineFeed, 0, 0);
        arg = transcodedMsg.section(QChar::LineFeed, 1);
        single = false;
    } else {
        var = transcodedMsg.section(QChar::Space, 0, 0);
        arg = transcodedMsg.section(QChar::Space, 1);
    }

    if (var.startsWith(QLatin1String("Client.Compose"))) {
        QString title;
        if (!single) {
            title = var.section(QChar::Space, 1);
        } else {
            title = arg;
            arg.clear();
        }

        if (mpComposer) {
            // If we already have the composer out then bail out:
            return;
        }

        mpComposer = new dlgComposer(mpHost);
        //FIXME
        if (arg.startsWith(QChar::Space)) {
            arg.remove(0, 1);
        }

        mpComposer->init(title, arg);
        mpComposer->raise();
        mpComposer->show();
        return;
    }

    var.remove(QLatin1Char('.'));
    arg.remove(QChar::LineFeed);
    int space = var.indexOf(' ');
    if (space > -1) {
        arg.prepend(QChar::Space);
        arg = arg.prepend(var.section(QChar::Space, 1));
        var = var.section(QChar::Space, 0, 0);
    }

    mpHost->mLuaInterpreter.setAtcpTable(var, arg);
    if (var.startsWith(QLatin1String("RoomNum"))) {
        if (mpHost->mpMap) {
            mpHost->mpMap->mRoomIdHash[mProfileName] = arg.toInt();
#if defined(INCLUDE_3DMAPPER)
            if (mpHost->mpMap->mpM && mpHost->mpMap->mpMapper) {
                mpHost->mpMap->mpM->update();
            }
#endif
            if (mpHost->mpMap->mpMapper && mpHost->mpMap->mpMapper->mp2dMap) {
                mpHost->mpMap->mpMapper->mp2dMap->update();
            }
        }
    }
}

// Helper function to parse the GUI version from JSON
QString cTelnet::parseGUIVersionFromJSON(const QJsonObject& json)
{
    QString version;
    auto versionJSON = json.value(qsl("version"));

    if (versionJSON != QJsonValue::Undefined && versionJSON.isString() && !versionJSON.toString().isEmpty()) {
        version = versionJSON.toString();
    } else if (versionJSON != QJsonValue::Undefined && versionJSON.isDouble()) {
        version = qsl("%1").arg(versionJSON.toInt());
    }

    return version;
}

// Helper function to parse the GUI URL from JSON
QString cTelnet::parseGUIUrlFromJSON(const QJsonObject& json)
{
    QString url;
    auto urlJSON = json.value(qsl("url"));

    if (urlJSON != QJsonValue::Undefined && !urlJSON.toString().isEmpty()) {
        url = urlJSON.toString();
    }

    return url;
}

// Helper function to download and install the GUI package
void cTelnet::downloadAndInstallGUIPackage(const QString& packageName, const QString& fileName, const QString& url)
{
    postMessage(tr("[ INFO ]  - Downloading and installing package '%1' (url='%2').").arg(packageName, url));

    mServerPackage = mudlet::getMudletPath(enums::profileDataItemPath, mProfileName, fileName);
    mpHost->updateProxySettings(mpDownloader);

    auto request = QNetworkRequest(QUrl(url));
    mudlet::self()->setNetworkRequestDefaults(url, request);
    mpPackageDownloadReply = mpDownloader->get(request);

    mpProgressDialog = new QProgressDialog(tr("Downloading game GUI from server..."), tr("Cancel"), 0, 4000000, mpHost->mpConsole);
    connect(mpPackageDownloadReply, &QNetworkReply::downloadProgress, this, &cTelnet::slot_setDownloadProgress);
    connect(mpProgressDialog, &QProgressDialog::canceled, mpPackageDownloadReply, &QNetworkReply::abort);
    mpProgressDialog->setAttribute(Qt::WA_DeleteOnClose);
    mpProgressDialog->show();
}

// Main logic for handling GUI package installation and upgrades
void cTelnet::handleGUIPackageInstallationAndUpgrade(QJsonDocument document)
{
    // Parse the JSON response
    auto json = document.object();
    if (json.isEmpty()) {
        return;
    }

    // Extract version and URL from JSON
    QString version = parseGUIVersionFromJSON(json);
    QString url = parseGUIUrlFromJSON(json);

    if (version.isEmpty() || url.isEmpty()) {
        return;  // Exit if version or URL is missing
    }

    // Clean up package name from URL
    QString packageName = url.section(QLatin1Char('/'), -1);
    QString fileName = packageName;

    packageName.remove(qsl(".zip"), Qt::CaseInsensitive);
    packageName.remove(qsl(".trigger"), Qt::CaseInsensitive);
    packageName.remove(qsl(".xml"), Qt::CaseInsensitive);
    packageName.remove(qsl(".mpackage"), Qt::CaseInsensitive);
    packageName.remove(QLatin1Char('/'));
    packageName.remove(QLatin1Char('\\'));
    packageName.remove(QLatin1Char('.'));

    // Check if the package is installed
    if (!mpHost->mInstalledPackages.contains(packageName)) {
        // Package is not installed, initiate the download
        mpHost->mServerGUI_Package_version = version;
        downloadAndInstallGUIPackage(packageName, fileName, url);
    } else if (mpHost->mServerGUI_Package_version != version) {
        // Check if the version is different and handle the upgrade
        postMessage(tr("[ INFO ]  - Upgrading the GUI to new version '%1' from version '%2' (url='%3').")
                    .arg(version, mpHost->mServerGUI_Package_version, url));

        // Uninstall the old version
        mpHost->uninstallPackage(mpHost->mServerGUI_Package_name != qsl("nothing") ? mpHost->mServerGUI_Package_name : packageName, enums::PackageModuleType::Package);

        // Download and install the new version
        mpHost->mServerGUI_Package_version = version;
        downloadAndInstallGUIPackage(packageName, fileName, url);
    }
}

void cTelnet::setGMCPVariables(const QByteArray& msg)
{
    // JSON (and thus the GMCP data) is always utf8
    QString transcodedMsg(msg);

    QString packageMessage;
    QString data;

    int firstNewline = transcodedMsg.indexOf(QChar::LineFeed);
    int firstSpace = transcodedMsg.indexOf(QChar::Space);

    // if we see a space before a newline, or no newlines at all,
    // then that's the separator for message and data
    if (Q_LIKELY((firstSpace != -1 && firstSpace < firstNewline) || firstNewline == -1)) {
        packageMessage = transcodedMsg.section(QChar::Space, 0, 0);
        data = transcodedMsg.section(QChar::Space, 1);
    } else {
        packageMessage = transcodedMsg.section(QChar::LineFeed, 0, 0);
        data = transcodedMsg.section(QChar::LineFeed, 1);
    }

    if (data.trimmed().isEmpty()) { // Example: Core.Ping
        // Pass empty table/object to Lua
        mpHost->mLuaInterpreter.setGMCPTable(packageMessage, qsl("{}"));
        return;
    }

    if (transcodedMsg.startsWith(qsl("Client.GUI"), Qt::CaseInsensitive)) {
        if (!mpHost->mAcceptServerGUI) {
            return;
        }

        // Mudlet supports two formats for parsing data associated with
        // Client.GUI package:
        //
        // JSON:       Client.GUI {"version": "39", "url": "https://www.example.com/example.mpackage"}
        // Raw Telnet: Client.GUI <version>\n<url>
        //
        // If the data does not parse as JSON, we'll try Raw telnet.

        auto document = QJsonDocument::fromJson(data.toUtf8());
        bool rawTelnet = false;

        if (!document.isObject()) {
            // Raw Telnet fallback
            QStringList lines = transcodedMsg.split(QChar::LineFeed);
            if (lines.size() < 2) {
                return;
            }

            QString version = lines[0].remove(QLatin1String("Client.GUI "), Qt::CaseInsensitive).trimmed();
            QString url = lines[1].trimmed();

            if (version.isEmpty() || url.isEmpty()) {
                return;
            }

            rawTelnet = true;
            document = QJsonDocument(QJsonObject{{"version", version}, {"url", url}});
        }

        handleGUIPackageInstallationAndUpgrade(document);

        if (rawTelnet) {
            return; // Do not add to the GMCP table
        }
    } else if (transcodedMsg.startsWith(QLatin1String("Client.Map"), Qt::CaseInsensitive)) {
        mpHost->setMmpMapLocation(data);
    }
    data.remove(QChar::LineFeed);
    // replace ANSI escape character with escaped version, to handle improperly passed ANSI codes
    // trying a different way of specifying the escape character
    data.replace(QLatin1String("\u001B"), QLatin1String("\\u001B"));
    // remove \r's from the data, as yajl doesn't like it
    data.remove(QChar::CarriageReturn);

    if (packageMessage.startsWith(QLatin1String("External.Discord.Status"), Qt::CaseInsensitive)
        || packageMessage.startsWith(QLatin1String("External.Discord.Info"), Qt::CaseInsensitive)) {
        mpHost->processDiscordGMCP(packageMessage, data);
    }

    if (mpHost->mAcceptServerMedia && packageMessage.startsWith(qsl("Client.Media"), Qt::CaseInsensitive)) {
        mpHost->mpMedia->parseGMCP(packageMessage, data);
    }

    if (packageMessage.startsWith(qsl("Char.Login"), Qt::CaseInsensitive)) {
        mpHost->mpAuth->handleAuthGMCP(packageMessage, data);
    }

    mpHost->mLuaInterpreter.setGMCPTable(packageMessage, data);
}

void cTelnet::setMSSPVariables(const QByteArray& msg)
{
    QString transcodedMsg;

    if (mpOutOfBandDataIncomingCodec) {
        // Message is encoded
        transcodedMsg = mpOutOfBandDataIncomingCodec->toUnicode(msg);
    } else {
        // Message is in ASCII (though this can handle Utf-8):
        transcodedMsg = msg;
    }

    transcodedMsg.remove(QChar::LineFeed);
    // replace ANSI escape character with escaped version, to handle improperly passed ANSI codes
    transcodedMsg.replace(QLatin1String("\u001B"), QLatin1String("\\u001B"));
    // remove \r's from the data, as yajl doesn't like it
    transcodedMsg.remove(QChar::CarriageReturn);

    mpHost->mLuaInterpreter.setMSSPTable(transcodedMsg);

#if !defined(QT_NO_SSL)
    promptTlsConnectionAvailable();
#endif
}

// Documentation: https://wiki.mudlet.org/w/Manual:Supported_Protocols#MSP
void cTelnet::setMSPVariables(const QByteArray& msg)
{
    QString transcodedMsg;

    if (mpOutOfBandDataIncomingCodec) {
        // Message is encoded
        transcodedMsg = mpOutOfBandDataIncomingCodec->toUnicode(msg);
    } else {
        // Message is in ASCII (though this can handle Utf-8):
        transcodedMsg = msg;
    }

    // MSP specification: https://www.zuggsoft.com/zmud/msp.htm#MSP%20Specification

    // remove \r and \n from the data.  They are part of the standard, but not needed.
    transcodedMsg.remove(QChar::CarriageReturn);
    transcodedMsg.remove(QChar::LineFeed);
    // replace ANSI escape character with escaped version, to handle improperly passed ANSI codes
    transcodedMsg.replace(QLatin1String("\u001B"), QLatin1String("\\u001B"));

    if (!transcodedMsg.endsWith(qsl(")"))) {
        return;
    } else {
        // Met the MSP standard so far. Remove this last right parenthesis.
        transcodedMsg.chop(1);
    }

    TMediaData mediaData {};

    mediaData.setMediaProtocol(TMediaData::MediaProtocolMSP);

    if (transcodedMsg.startsWith(qsl("!!SOUND("))) {
        mediaData.setMediaType(TMediaData::MediaTypeSound);
        transcodedMsg.remove(qsl("!!SOUND("));
    } else if (transcodedMsg.startsWith(qsl("!!MUSIC("))) {
        mediaData.setMediaType(TMediaData::MediaTypeMusic);
        transcodedMsg.remove(qsl("!!MUSIC("));
    } else {
        // Does not meet the MSP standard.
        return;
    }

    if (transcodedMsg == "Off") {
        mpHost->mpMedia->stopMedia(mediaData);
        return;
    }

    QStringList argumentList = transcodedMsg.split(QChar::Space);

    if (argumentList.size() > 0) {
        for (int i = 0; i < argumentList.size(); ++i) {
            if (i < 1) {
                mediaData.setMediaFileName(argumentList[i]);
            } else {
                QStringList payloadList = argumentList[i].split('=');

                if (payloadList.size() != 2) {
                    return; // Invalid MSP.
                }

                QString mspVAR;
                QString mspVAL;

                for (int j = 0; j < payloadList.size(); ++j) {
                    if (j < 1) {
                        mspVAR = payloadList[j];
                    } else {
                        mspVAL = payloadList[j];
                    }
                }

                if (mspVAR == "V") {
                    mediaData.setMediaVolume(mspVAL.toInt());
                } else if (mspVAR == "L") {
                    mediaData.setMediaLoops(mspVAL.toInt());

                    if (mediaData.mediaLoops() < TMediaData::MediaLoopsRepeat || mediaData.mediaLoops() == 0) {
                        mediaData.setMediaLoops(TMediaData::MediaLoopsDefault);
                    }
                } else if (mspVAR == "P") {
                    mediaData.setMediaPriority(mspVAL.toInt());

                    if (mediaData.mediaPriority() > TMediaData::MediaPriorityMax) {
                        mediaData.setMediaPriority(TMediaData::MediaPriorityMax);
                    } else if (mediaData.mediaPriority() < TMediaData::MediaPriorityMin) {
                        mediaData.setMediaPriority(TMediaData::MediaPriorityMin);
                    }
                } else if (mspVAR == "C") {
                    if (mspVAL.toInt() == 0) {
                        mediaData.setMediaContinue(TMediaData::MediaContinueRestart);
                    } else {
                        mediaData.setMediaContinue(TMediaData::MediaContinueDefault);
                    }
                } else if (mspVAR == "T") {
                    mediaData.setMediaTag(mspVAL.toLower()); // To provide case insensitivity of MSP spec
                } else if (mspVAR == "U") {
                    mediaData.setMediaUrl(mspVAL);
                } else {
                    qDebug() << "MSP: tag" << mspVAR << "isn't one we understand";
                    continue; // robustness principle: ignore anything we don't understand
                }
            }
        }
    }

    mpHost->mpMedia->playMedia(mediaData);
}

bool cTelnet::isIPAddress(QString& arg)
{
    bool isIPAddress = false;

    QHostAddress address(arg);

    if (QAbstractSocket::IPv4Protocol == address.protocol()) {
        isIPAddress = true;
    } else if (QAbstractSocket::IPv6Protocol == address.protocol()) {
        isIPAddress = true;
    }

    return isIPAddress;
}

#if !defined(QT_NO_SSL)
void cTelnet::promptTlsConnectionAvailable()
{
    // If an SSL port is detected by MSSP and we're not using it, prompt to use on future connections
    if (mpHost->mMSSPTlsPort && socket.mode() == QSslSocket::UnencryptedMode && mpHost->mAskTlsAvailable && !isIPAddress(hostName)
        && (mpHost->mMSSPHostName.isEmpty() || QString::compare(hostName, mpHost->mMSSPHostName, Qt::CaseInsensitive) == 0)) {
        postMessage(tr("[ INFO ]  - A more secure connection on port %1 is available.").arg(QString::number(mpHost->mMSSPTlsPort)));

        auto msgBox = new QMessageBox();

        msgBox->setIcon(QMessageBox::Question);
        msgBox->setText(tr("For data transfer protection and privacy, this connection advertises a secure port."));
        msgBox->setInformativeText(tr("Update to port %1 and connect with encryption?").arg(QString::number(mpHost->mMSSPTlsPort)));
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::Yes);

        int ret = msgBox->exec();
        delete msgBox;

        switch (ret) {
        case QMessageBox::Yes:
            cTelnet::disconnectIt();
            hostPort = mpHost->mMSSPTlsPort;
            mpHost->setPort(hostPort);
            mpHost->mSslTsl = true;
            mpHost->writeProfileData(QLatin1String("port"), QString::number(hostPort));
            mpHost->writeProfileData(QLatin1String("ssl_tsl"), QString::number(Qt::Checked));
            cTelnet::connectIt(mpHost->getUrl(), hostPort);
            break;
        case QMessageBox::No:
            cTelnet::disconnectIt();
            mpHost->mAskTlsAvailable = false; // Don't ask next time
            cTelnet::reconnect();             // A no-op (;) is desired, but read buffer does not flush
            break;
        default:
            // should never be reached
            break;
        }
    }
}
#endif

bool cTelnet::purgeMediaCache()
{
    return mpHost->mpMedia->purgeMediaCache();
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

void cTelnet::setAutoReconnect(bool status)
{
    mAutoReconnect = status;
}

void cTelnet::atcpComposerCancel()
{
    if (!mpComposer) {
        return;
    }
    mpComposer->close();
    mpComposer = nullptr;
    // This will be unaffected by Mud Server encoding:
    std::string output = "*q\nno\n";
    socketOutRaw(output);
}

void cTelnet::atcpComposerSave(QString txt)
{
    if (!mpHost->mEnableGMCP) {
        if (enableATCP) {
            //olesetbuf \n <text>
            std::string output;
            output += TN_IAC;
            output += TN_SB;
            output += OPT_ATCP;
            output += "olesetbuf \n ";
            output += encodeAndCookBytes(txt.toStdString());
            output += '\n';
            output += TN_IAC;
            output += TN_SE;
            socketOutRaw(output);

            output = "*s\n";
            socketOutRaw(output);
        }

    } else if (enableGMCP) {
        std::string output;
        output += TN_IAC;
        output += TN_SB;
        output += OPT_GMCP;
        output += "IRE.Composer.SetBuffer";
        if (!txt.isEmpty()) {
            // Escape the text for the GMCP message that we will send to the game, put the result inside of quotes.
            // Backslashes are escaped first because they are contained in the others, then quotes and new lines.
            output += " \"";
            output += encodeAndCookBytes(txt.replace('\\', QLatin1String(R"(\\)")).replace('\"', QLatin1String(R"(\")")).replace('\n', QLatin1String(R"(\n)")).toStdString());
            output += "\"";
        }
        output += TN_IAC;
        output += TN_SE;
        socketOutRaw(output);

        output = "*s\n";
        socketOutRaw(output);
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
// in existence as happens during startup, then pumps them out in order of
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

        qint8 openBraceIndex = body.at(0).indexOf(QLatin1String("["));
        qint8 closeBraceIndex = body.at(0).indexOf(QLatin1String("]"));
        qint8 hyphenIndex = body.at(0).indexOf(QLatin1String("- "));
        if (openBraceIndex >= 0 && closeBraceIndex > 0 && closeBraceIndex < hyphenIndex) {
            quint8 prefixLength = hyphenIndex + 1;
            while (body.at(0).at(prefixLength) == ' ') {
                ++prefixLength;
            }

            QString prefix = body.at(0).left(prefixLength).toUpper();
            QString firstLineTail = body.at(0).mid(prefixLength);
            body.removeFirst();
            //: Keep the capitalisation, the translated text at 7 letters max so it aligns nicely
            if (prefix.contains(tr("ERROR")) || prefix.contains(QLatin1String("ERROR"))) {
                mpHost->mpConsole->print(prefix, Qt::red, mpHost->mBgColor);                                  // Bright Red
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(255, 255, 50), mpHost->mBgColor); // Bright Yellow
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    // Fix for lua using tabs for indentation which was messing up justification:
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(255, 255, 50), mpHost->mBgColor); // Bright Yellow
                }
            //: Keep the capisalisation, the translated text at 7 letters max so it aligns nicely
            } else if (prefix.contains(tr("LUA")) || prefix.contains(QLatin1String("LUA"))) {
                mpHost->mpConsole->print(prefix, QColor(80, 160, 255), mpHost->mBgColor);                    // Light blue
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(50, 200, 50), mpHost->mBgColor); // Light green
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(200, 50, 50), mpHost->mBgColor); // Red
                }
            //: Keep the capisalisation, the translated text at 7 letters max so it aligns nicely
            } else if (prefix.contains(tr("WARN")) || prefix.contains(QLatin1String("WARN"))) {
                mpHost->mpConsole->print(prefix, QColor(0, 150, 190), mpHost->mBgColor);                     // Cyan
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(190, 150, 0), mpHost->mBgColor); // Orange
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 150, 0), mpHost->mBgColor);
                }
            //: Keep the capisalisation, the translated text at 7 letters max so it aligns nicely
            } else if (prefix.contains(tr("ALERT")) || prefix.contains(QLatin1String("ALERT"))) {
                mpHost->mpConsole->print(prefix, QColor(190, 100, 50), mpHost->mBgColor);                     // Orange-ish
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(190, 190, 50), mpHost->mBgColor); // Yellow
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 190, 50), mpHost->mBgColor); // Yellow
                }
            //: Keep the capisalisation, the translated text at 7 letters max so it aligns nicely
            } else if (prefix.contains(tr("INFO")) || prefix.contains(QLatin1String("INFO"))) {
                mpHost->mpConsole->print(prefix, QColor(0, 150, 190), mpHost->mBgColor);                   // Cyan
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(0, 160, 0), mpHost->mBgColor); // Light Green
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(0, 160, 0), mpHost->mBgColor); // Light Green
                }
            //: Keep the capisalisation, the translated text at 7 letters max so it aligns nicely
            } else if (prefix.contains(tr("OK")) || prefix.contains(QLatin1String("OK"))) {
                mpHost->mpConsole->print(prefix, QColor(0, 160, 0), mpHost->mBgColor);                        // Light Green
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(190, 100, 50), mpHost->mBgColor); // Orange-ish
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 100, 50), mpHost->mBgColor); // Orange-ish
                }
            } else {                                                                                        // Unrecognised but still in a "[ something ] -  message..." format
                mpHost->mpConsole->print(prefix, QColor(190, 50, 50), mpHost->mBgColor);                    // Foreground red, background bright grey
                mpHost->mpConsole->print(firstLineTail.append('\n'), QColor(50, 50, 50), mpHost->mBgColor); //Foreground dark grey, background bright grey
                for (int _i = 0; _i < body.size(); ++_i) {
                    QString temp = body.at(_i);
                    temp.replace('\t', QLatin1String("        "));
                    body[_i] = temp.rightJustified(temp.length() + prefixLength);
                }
                if (!body.empty()) {
                    mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(50, 50, 50), mpHost->mBgColor); //Foreground dark grey, background bright grey
                }
            }
        } else {                                                                                             // No prefix found
            mpHost->mpConsole->print(body.join('\n').append('\n'), QColor(190, 190, 190), mpHost->mBgColor); //Foreground bright grey
        }
        messageStack.removeFirst();
    }
}

//forward data for further processing


void cTelnet::gotPrompt(std::string& mud_data)
{
    mpPostingTimer->stop();
    if (mpPostingTimer->interval() != mTimeOut) {
        mpPostingTimer->setInterval(mTimeOut);
    }
    mMudData += mud_data;

    if (mUSE_IRE_DRIVER_BUGFIX && mGA_Driver) {
        //////////////////////////////////////////////////////////////////////
        //
        // Patch for servers that need GA/EOR for prompt fixups
        //

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
                    ++j;
                }
            }
            if (mMudData[j] == '\n') {
                mMudData.erase(j, 1);
                break;
            } else {
                break;
            }
        NEXT:
            ++j;
        }
        //
        ////////////////////////////
    }

    postData();
    mMudData = "";
    mIsTimerPosting = false;
}

void cTelnet::gotRest(std::string& mud_data)
{
    if (mud_data.empty()) {
        return;
    }
    if (!mGA_Driver) {
        size_t i = mud_data.rfind('\n');
        if (i != std::string::npos) {
            mMudData += mud_data.substr(0, i + 1);
            postData();
            if (!mIsTimerPosting && (mpPostingTimer->interval() != mTimeOut)) {
                mpPostingTimer->setInterval(mTimeOut);
            }
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
                if (mpPostingTimer->interval() != mTimeOut) {
                    mpPostingTimer->setInterval(mTimeOut);
                }
                mpPostingTimer->start();
                mIsTimerPosting = true;
            }
        }

    } else {
        mMudData += mud_data;
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

    mZstream.avail_out = BUFFER_SIZE;
    mZstream.next_out = (Bytef*)out_buffer;

    int zval = inflate(&mZstream, Z_SYNC_FLUSH);
    int outSize = BUFFER_SIZE - mZstream.avail_out;

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

        // We shouldn't return -1 or an error here, as that prevents any text
        // or any telnet negotiation strings from being properly interpreted
        // by Mudlet, and shown to the user.
        // Returning outSize ensures anything sent before the Z_STREAM_END is
        // shown to the user.
    }
    return outSize;
}


void cTelnet::recordReplay()
{
    mRecordLastChunkMSecTimeOffset = 0;
    mRecordingChunkTimer.start();
    mRecordingChunkCount = 0;
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
            mIsReplayRunFromLua = false;
        } else {
            mIsReplayRunFromLua = true;
        }
        replayStream.setDevice(&replayFile);
        if (QVersionNumber::fromString(QString(qVersion())) >= QVersionNumber(5, 13, 0)) {
            replayStream.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        loadingReplay = true;
        if (mudlet::self()->replayStart()) {
            auto [ok, modifiedFormat] = testReadReplayFile();
            if (Q_LIKELY(ok)) {
                mReplayHasFaultyFormat = modifiedFormat;
                // This initiates the replay chunk reading/processing cycle:
                loadReplayChunk();
            } else {
                // Amelioration code should now prevent this from happening
                loadingReplay = false;
                replayFile.close();
                if (pErrMsg) {
                    // Called from lua case:
                    *pErrMsg = tr("Cannot replay file \"%1\", error message was: \"replay file seems to be corrupt\".").arg(name);
                } else {
                    postMessage(tr("[ WARN ]  - The replay has been aborted as the file seems to be corrupt."));
                }
                mudlet::self()->replayOver();
                return false;
            }

        } else {
            loadingReplay = false;
            if (pErrMsg) {
                *pErrMsg = tr("Cannot perform replay, another one may already be in progress. Try again when it has finished.");
            } else {
                postMessage(tr("[ WARN ]  - Cannot perform replay, another one may already be in progress.\n"
                               "Try again when it has finished."));
            }
            return false;
        }
    } else {
        if (pErrMsg) {
            // Call from lua case:
            *pErrMsg = tr("Cannot read file \"%1\", error message was: \"%2\".")
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

// TODO: https://github.com/Mudlet/Mudlet/issues/5779 - consider enhancing replay system, possibly using the QTimeLine class
void cTelnet::loadReplayChunk()
{
    if (!replayStream.atEnd()) {
        qint32 amount = 0;
        qint32 offset = 0;
        if (mReplayHasFaultyFormat) {
            qint64 temp = 0;
            replayStream >> temp;
            // 2^30 milliseconds is over 12 days so that sort of delay between
            // steps is not likely - and only using a 32 bit integer type is
            // going to be okay:
            offset = static_cast<qint32>(temp);
        } else {
            replayStream >> offset;
        }

        replayStream >> amount;

        loadedBytes = replayStream.readRawData(loadBuffer, amount);
        // Previous use of loadedBytes + 1 caused a spurious character at end of
        // string display by a qDebug of the loadBuffer contents
        loadBuffer[loadedBytes] = '\0';
        mudlet::self()->mReplayTime = mudlet::self()->mReplayTime.addMSecs(offset);
        QTimer::singleShot(offset / mudlet::self()->mReplaySpeed, this, &cTelnet::slot_processReplayChunk);
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
    std::string cleandata = "";
    recvdGA = false;
    for (int i = 0; i < datalen; ++i) {
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
                processTelnetCommand(command);
                //this could have set receivedGA to true; we'll handle that later
                command = "";
            }
        } else {
            if (ch != '\r' && ch != '\0') {
                cleandata += ch;
            }
        }

        if (recvdGA) {
            mGA_Driver = true;
            if (mCommands > 0) {
                mCommands--;
                if (networkLatencyTimer.elapsed() > 2000) {
                    mCommands = 0;
                }
            }

            cleandata.push_back('\n');
            recvdGA = false;
            gotPrompt(cleandata);
            cleandata = "";
        }
    } //for

    if (!cleandata.empty()) {
        gotRest(cleandata);
    }

    mpHost->mpConsole->finalize();
    if (loadingReplay) {
        loadReplayChunk();
    }
}

void cTelnet::slot_socketReadyToBeRead()
{
    if (mWaitingForResponse) {
        networkLatencyTime = networkLatencyTimer.elapsed() / 1000.0;
        mWaitingForResponse = false;
    }

    // TODO: https://github.com/Mudlet/Mudlet/issues/5780 (2 of 7) - investigate switching from using `char[]` to `std::array<char>`
    char in_buffer[BUFFER_SIZE + 10];

    int amount = socket.read(in_buffer, BUFFER_SIZE);
    processSocketData(in_buffer, amount);
}

void cTelnet::processSocketData(char* in_buffer, int amount, const bool loopbackTesting)
{
    // TODO: https://github.com/Mudlet/Mudlet/issues/5780 (3 of 7) - investigate switching from using `char[]` to `std::array<char>`
    char out_buffer[BUFFER_SIZE + 10];

    in_buffer[amount + 1] = '\0';
    if (amount == -1) {
        return;
    }
    if (amount == 0) {
        return;
    }

    std::string cleandata = "";
    qint32 datalen = 0;
    do {
        datalen = amount;
        char* buffer = in_buffer;
        if (mNeedDecompression) {
            datalen = decompressBuffer(in_buffer, amount, out_buffer);
            buffer = out_buffer;
        }
        // TODO: https://github.com/Mudlet/Mudlet/issues/5780 (4 of 7) - investigate switching from using `char[]` to `std::array<char>`
        buffer[static_cast<size_t>(datalen)] = '\0';
        if (!loopbackTesting && mpHost->mpConsole->mRecordReplay) {
            ++mRecordingChunkCount;
            // QElapsedTimer::elapsed() returns a qint64, it replaces a
            // previous QTime::elapsed() which returns a int (effectively a
            // qint32):
            qint32 recordingChunkInterval = static_cast<qint32>(mRecordingChunkTimer.elapsed()) - mRecordLastChunkMSecTimeOffset;
            mpHost->mpConsole->mReplayStream << recordingChunkInterval; // 4 bytes
            mpHost->mpConsole->mReplayStream << datalen;                // 4 bytes
            mpHost->mpConsole->mReplayStream.writeRawData(buffer, datalen);
#if defined(DEBUG_RECORDING)
            qDebug().noquote().nospace() << "cTelnet::processSocketData(...) INFO - recording chunk: " << mRecordingChunkCount << " is " << datalen
                                         << " bytes and has an interval of: " << recordingChunkInterval << " mSecond since the previous chunk.";
#endif
        }

        recvdGA = false;
        for (int i = 0; i < datalen; ++i) {
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
                    // IAC SB COMPRESS WILL SE for MCCP v1 (unterminated invalid telnet sequence)
                    // IAC SB COMPRESS2 IAC SE for MCCP v2
                    if ((mMCCP_version_1 || mMCCP_version_2) && (!mNeedDecompression)) {
                        // TODO this code looks ahead instead of using the state machine.
                        // This is not a good idea.
                        char _ch = buffer[i];
                        if ((_ch == OPT_COMPRESS) || (_ch == OPT_COMPRESS2)) {
                            bool _compress = false;
                            if ((i > 1) && (i + 2 < datalen)) {
                                if ((buffer[i - 2] == TN_IAC) && (buffer[i - 1] == TN_SB) && (buffer[i + 1] == TN_WILL) && (buffer[i + 2] == TN_SE)) {
                                    qDebug() << "MCCP version 1 starting sequence";
                                    _compress = true;
                                }
                                if ((buffer[i - 2] == TN_IAC) && (buffer[i - 1] == TN_SB) && (buffer[i + 1] == TN_IAC) && (buffer[i + 2] == TN_SE)) {
                                    qDebug() << "MCCP version 2 starting sequence";
                                    _compress = true;
                                }
                            }
                            if (_compress) {
                                mNeedDecompression = true;
                                // from this position in stream onwards, data will be compressed by zlib
                                gotRest(cleandata);
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
                                // compressed data starts in clean state
                                iac = false;
                                insb = false;
                                command = "";
                                goto MAIN_LOOP_END;
                            }
                        }
                    }
                    //7. inside IAC SB

                    command += ch;
                    if (iac && (ch == TN_SE)) { //IAC SE - end of subcommand
                        processTelnetCommand(command);
                        command = "";
                        iac = false;
                        insb = false;
                    } else if (iac && (ch == TN_IAC)) { // escaped TN_IAC
                        command.pop_back();
                        iac = false;
                    } else if (iac) {
                        // Telnet options within a subcommand are not supported.
                        // We assume that the SE went missing, possibly due to a
                        // server bug, and try to recover.
                        // Cf. https://github.com/Mudlet/Mudlet/issues/4385
                        command.pop_back();
                        command += TN_SE;
                        processTelnetCommand(command);
                        if (!mIncompleteSB) {
                            mIncompleteSB = true;
                            qWarning(R"("TELNET: the server did not properly complete a subnegotiation (code %02x).
Some data loss is likely - please mention this problem to the game admins.)", command[2]);
                        }


                        // Re-enter the state machine.
                        command = TN_IAC;
                        iac = true;
                        insb = false;
                        i -= 1;
                    } else if (ch == TN_IAC) {
                        iac = true;
                    }
                } else
                //8. IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
                {
                    iac = false;
                    command += ch;
                    processTelnetCommand(command);
                    //this could have set receivedGA to true; we'll handle that later
                    command = "";
                }
            } else {
                if (ch == TN_BELL) {
                    // Flash taskbar for 3 seconds on the telnet bell, note
                    // by processing it here rather than in the TTextEdit class
                    // it is not possible to fake/test it with a Lua
                    // feedTriggers(...) call - OTOH doing it there would make
                    // a beep every time the screen was refreshed!
                    // TODO: https://github.com/Mudlet/Mudlet/issues/5836 - provide option to actually make a (void) QApplication::beep() or a user-selected sound (different for each profile) and/or instead of the visual alert
                    QApplication::alert(mudlet::self(), 3000);
                }
                if (ch != '\r' && ch != '\0') {
                    cleandata += ch;
                }
            }
        MAIN_LOOP_END:;
            if (recvdGA) {
                if (!mFORCE_GA_OFF) //FIXME: isn't initialized correctly
                {
                    mGA_Driver = true;
                    if (mCommands > 0) {
                        mCommands--;
                        if (networkLatencyTimer.elapsed() > 2000) {
                            mCommands = 0;
                        }
                    }
                    cleandata.push_back('\xff');
                    recvdGA = false;
                    gotPrompt(cleandata);
                    cleandata = "";
                } else {
                    cleandata.push_back('\n');
                }
            }
        } //for
    } while (datalen == BUFFER_SIZE);

    if (!cleandata.empty()) {
        gotRest(cleandata);
    }
    mpHost->mpConsole->finalize();
    mRecordLastChunkMSecTimeOffset = mRecordingChunkTimer.elapsed();
}

void cTelnet::raiseProtocolEvent(const QString& name, const QString& protocol)
{
    TEvent event {};
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
#if defined(Q_OS_WINDOWS)
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

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_OPENBSD)
    setsockopt(socketHandle, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#else
    // FreeBSD always has the Keep-alive option enabled, so the above is not
    // usable
    Q_UNUSED(on)
#endif

    // The effect is that (on FreeBSD) "init" seconds is allowed to set up the
    // connection, then after (all OSes) "timeout" seconds with no traffic a
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
#elif defined(Q_OS_OPENBSD)
    // There does not appear to be a per-socket option for TCP_KEEPALIVE on OpenBSD
    // only a system wide one
#else
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPIDLE, &timeout, sizeof(timeout));
#endif

#if !defined(Q_OS_OPENBSD)
    // There does not appear to be a per-socket options for these on OpenBSD
    // only system wide one:

    // Interval between keep-alives, in seconds:
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    // Number of failed keep alives before forcing a close:
    setsockopt(socketHandle, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
#endif // !defined(Q_OS_OPENBSD)
#endif // !defined(Q_OS_WINDOWS)
}

// Used to convert a collection of Bytes in the current MUD Server encoding
// to the UTF-16BE encoding used for QString and then back to a series of bytes
// as a QByteArray - note that it does NOT retain states between calls as it
// assumes each call is a complete separate chunk of text - should this not
// prove to be the case in practice it will be necessary to fork-off separate
// instances of this method for each OOB protocol that uses this DECODER:
QByteArray cTelnet::decodeBytes(const char* bytes)
{
    if (mpOutOfBandDataIncomingCodec) {
        // (QString) QTextCodec::toUnicode(const char *chars) const converts
        // from given encoding to the QString UTF-16BE Unicode form:
        return mpOutOfBandDataIncomingCodec->toUnicode(bytes).toUtf8().constData();
    } else {
        return QByteArray(bytes);
    }
}

// Converts a Unicode (UTF-8) encoded std::string into the current Mud Server
// encoding and cooks any 0xff bytes by doubling them to get them through Telnet
// protocol handling in the Server - this is needed, at least, for the following
//  characters in the following encodings which WILL become the 0xff value:
// 'ÿ' {U+00FF Latin small letter y with diaeresis} ==> ISO 8859-1/9/14/15/16
// '˙' {U+02D9 Dot above}                          ==> ISO 8859-2/3/4
// 'џ' {U+045F Cyrillic small letter dzhe}         ==> ISO 8859-5
// 'ĸ' {U+0138 Latin small letter kra}             ==> ISO 8859-10
// '’' {U+2019 Right single quotation mark}        ==> ISO 8859-13
// '<nbsp>' {U+00A0 Non-breaking space}            ==> CP-850
std::string cTelnet::encodeAndCookBytes(const std::string& data)
{
    if (mpOutOfBandDataIncomingCodec) {
        // QTextCodec::fromUnicode(...) converts from QString in UTF16BE
        // encoding to the required Mud Server encoding as a QByteArray,
        // QString::fromStdString(...) converts from a UTF8 encoded std::string
        // to a UTF16BE encoded QString:
        return mudlet::replaceString(mpOutOfBandDataIncomingCodec->fromUnicode(QString::fromStdString(data)).toStdString(), "\xff", "\xff\xff");
    } else {
        // std::string::c_str() converts the std::string into a char array WITH
        // a garenteed terminating null byte.
        return mudlet::replaceString(data, "\xff", "\xff\xff");
    }
}

void cTelnet::setPostingTimeout(const int timeout)
{
    if (mTimeOut != timeout) {
        mTimeOut = timeout;
    }
}

// Tries reading the replay in two different manners depending on whether the
// the first integer value in the chunk data uses 4 (original) or 8 (modified)
// bytes - as an unintended side effect of https://github.com/Mudlet/Mudlet/pull/4400
// - returns two booleans, the first is true if the file can be read and the
// second true if it is in the modified format:
/*static*/ std::pair<bool, bool> cTelnet::testReadReplayFile()
{
    // TODO: https://github.com/Mudlet/Mudlet/issues/5780 (5 of 7) - investigate switching from using `char[]` to `std::array<char>`
    char replayBuffer[BUFFER_SIZE+1];

    quint64 totalElapsed = 0;
    int replayChunks = 0;
    bool readableAsOriginalFormat = true;
    // Don't set this until we try it:
    bool readableAsModifiedFormat = false;
    {
        // Try with both numbers being 4 byte signed integers
        // (first was int type prior to that PR):
        qint32 offset = 0;
        qint32 amount = 0;
        while (readableAsOriginalFormat && !replayStream.atEnd()) {
            replayStream >> offset;
            replayStream >> amount;
            if (amount < 1 || offset < 0 || amount > static_cast<qint32>(BUFFER_SIZE)) {
                readableAsOriginalFormat = false;
            } else {
                int replayloadedBytes = replayStream.readRawData(replayBuffer, amount);
                if (replayloadedBytes > -1) {
                    ++replayChunks;
                    // TODO: https://github.com/Mudlet/Mudlet/issues/5780 (6 of 7) - investigate switching from using `char[]` to `std::array<char>`
                    replayBuffer[replayloadedBytes] = '\0';
                    totalElapsed += static_cast<quint64>(offset);
                }
            }
        }
    }

    // rewind the data to the start as if we haven't just read some/all of it
    replayStream.device()->seek(0);

    if (!readableAsOriginalFormat) {
        readableAsModifiedFormat = true;
        totalElapsed = 0;
        replayChunks = 0;
        // Try with first number being an 8 byte signed integer
        // (was int type prior to that PR):
        qint64 offset = 0;
        qint32 amount = 0;
        while (readableAsModifiedFormat && !replayStream.atEnd()) {
            replayStream >> offset;
            replayStream >> amount;
            if (amount < 1 || offset < 0 || amount > static_cast<qint32>(BUFFER_SIZE) || offset > INT32_MAX) {
                readableAsModifiedFormat = false;
            } else {
                int replayloadedBytes = replayStream.readRawData(replayBuffer, amount);
                if (replayloadedBytes > -1) {
                    ++replayChunks;
                    // TODO: https://github.com/Mudlet/Mudlet/issues/5780 (7 of 7) - investigate switching from using `char[]` to `std::array<char>`
                    replayBuffer[replayloadedBytes] = '\0';
                    totalElapsed += static_cast<quint64>(offset);
                }
            }
        }

        replayStream.device()->seek(0);
    }

    if (readableAsOriginalFormat | readableAsModifiedFormat) {
        qDebug().nospace().noquote() << "cTelnet::testReadReplayFile() INFO - The " << (readableAsOriginalFormat ? "original" : "modified") << " format replay has: " << replayChunks
                                     << " chunks and covers a period of: " << QTime(0, 0).addMSecs(static_cast<int>(totalElapsed)).toString(qsl("hh:mm:ss.zzz")) << " (hh:mm:ss).";

        return {true, readableAsModifiedFormat};
    }

    return {false, false};
}
