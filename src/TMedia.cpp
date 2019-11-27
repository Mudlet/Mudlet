/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2019 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TMedia.h"

// Public
TMedia::TMedia(Host* pHost, const QString& profileName)
{
    mpHost = pHost;
    mProfileName = profileName;

    mpNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mpNetworkAccessManager, &QNetworkAccessManager::finished, this, &TMedia::writeFile);
}

TMedia::~TMedia() {}

// Documentation: https://wiki.mudlet.org/w/Manual:Supported_Protocols#MSP
void TMedia::parseGMCP(QString& packageMessage, QString& gmcp)
{
    if (!mpHost->mAcceptServerMedia) {
        return;
    }

    auto document = QJsonDocument::fromJson(gmcp.toUtf8());

    if (!document.isObject()) {
        return;
    }

    // This is JSON
    auto json = document.object();

    if (json.isEmpty()) {
        return;
    }

    if (packageMessage == "Client.Media.Load") {
        TMedia::parseJSONForMediaLoad(json);
    } else if (packageMessage == "Client.Media.Play") {
        TMedia::parseJSONForMediaPlay(json);
    } else if (packageMessage == "Client.Media.Stop") {
        TMedia::parseJSONForMediaStop(json);
    }
}

void TMedia::playMedia(TMediaData& mediaData)
{
    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    mediaData.setMediaFileName(mediaData.getMediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    if (!TMedia::isFileRelative(mediaData) || mediaData.getMediaTag().contains("..")) { // Security
        return;
    }

    QUrl url = TMedia::parseUrl(mediaData);

    if (!TMedia::isValidUrl(url)) {
        return;
    } else if (mpHost->getMediaLocation().isEmpty() || url.toString() != mpHost->getMediaLocation()) {
        mpHost->setMediaLocation(url.toString());
    }

    if (mediaData.getMediaFileName() == "Off") {
        return;
    }

    QString absolutePathFileName;

    if (!mediaData.getMediaFileName().contains('*') && !mediaData.getMediaFileName().contains('?')) { // File path wildcards are * and ?
        if (!mediaData.getMediaFileName().contains('.')) {
            switch (mediaData.getMediaType()) {
                case TMediaData::MediaTypeSound:
                    mediaData.setMediaFileName(mediaData.getMediaFileName().append(".wav"));
                    break;
                case TMediaData::MediaTypeMusic:
                    mediaData.setMediaFileName(mediaData.getMediaFileName().append(".mid"));
                    break;
            }
        }

        if (!mediaData.getMediaTag().isEmpty()) {
            absolutePathFileName = QStringLiteral("%1/%2/%3")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag(), mediaData.getMediaFileName());
        } else {
            absolutePathFileName = QStringLiteral("%1/%2")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());
        }

        mediaData.setMediaAbsolutePathFileName(absolutePathFileName);

        QFile mediaFile(absolutePathFileName);

        if (!mediaFile.exists()) {
            TMedia::downloadFile(mediaData);
            return;
        }
    }

    switch (mediaData.getMediaType()) {
        case TMediaData::MediaTypeSound:
            TMedia::playSound(mediaData);
            break;
        case TMediaData::MediaTypeMusic:
            TMedia::playMusic(mediaData);
            break;
        case TMediaData::MediaTypeVideo:
            return;
    }
}

void TMedia::stopMedia(TMediaData& mediaData)
{
    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    QList<TMediaPlayer*> mTMediaPlayerList;

    switch (mediaData.getMediaProtocol()) {
        case TMediaData::MediaProtocolMSP:
            switch (mediaData.getMediaType()) {
                case TMediaData::MediaTypeSound:
                    mTMediaPlayerList = mMSPSoundList;
                    break;
                case TMediaData::MediaTypeMusic:
                    mTMediaPlayerList = mMSPMusicList;
                    break;
            }
            break;

        case TMediaData::MediaProtocolGMCP:
            switch (mediaData.getMediaType()) {
                case TMediaData::MediaTypeSound:
                    mTMediaPlayerList = mGMCPSoundList;
                    break;
                case TMediaData::MediaTypeMusic:
                    mTMediaPlayerList = mGMCPMusicList;
                    break;
                case TMediaData::MediaTypeVideo:
                    mTMediaPlayerList = mGMCPVideoList;
                    break;
                case TMediaData::MediaTypeNotSet:
                    mTMediaPlayerList = mGMCPSoundList + mGMCPMusicList + mGMCPVideoList;
                    break;
            }
            break;

        default:
            return;
    }

    QListIterator<TMediaPlayer*> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer* pPlayer = itTMediaPlayer.next();

        if (mediaData.getMediaProtocol() == TMediaData::MediaProtocolGMCP) {
            if (!mediaData.getMediaKey().isEmpty() && !pPlayer->getMediaData().getMediaKey().isEmpty()
                && pPlayer->getMediaData().getMediaKey() != mediaData.getMediaKey()) {
                continue;
            }

            if (!mediaData.getMediaFileName().isEmpty() && !pPlayer->getMediaData().getMediaFileName().isEmpty()
                && pPlayer->getMediaData().getMediaFileName() != mediaData.getMediaFileName()) {
                continue;
            }

            if (!mediaData.getMediaTag().isEmpty() && !pPlayer->getMediaData().getMediaTag().isEmpty()
                && pPlayer->getMediaData().getMediaTag() != mediaData.getMediaTag()) {
                continue;
            }

            if (!mediaData.getMediaTarget().isEmpty() && !pPlayer->getMediaData().getMediaTarget().isEmpty()
                && pPlayer->getMediaData().getMediaTarget() != mediaData.getMediaTarget()) {
                continue;
            }

            if (mediaData.getMediaPriority() != TMediaData::MediaPriorityNotSet && pPlayer->getMediaData().getMediaPriority() != TMediaData::MediaPriorityNotSet
                && pPlayer->getMediaData().getMediaPriority() <= mediaData.getMediaPriority()) {
                continue;
            }
        }

        pPlayer->getMediaPlayer()->stop();
    }
}
// End Public

// Private
QUrl TMedia::parseUrl(TMediaData& mediaData)
{
    QUrl url;

    if (mediaData.getMediaFileName() == "Off") {
        if (mediaData.getMediaUrl().isEmpty()) { // MSP is !!SOUND(Off) or !!MUSIC(Off)
            mpHost->mpMedia->stopMedia(mediaData);
        } else { // MSP is !!SOUND(Off U=https://example.com/sounds) or !!MUSIC(Off U=https://example.com/sounds)
            url = QUrl::fromUserInput(mediaData.getMediaUrl());
        }
    } else if (mediaData.getMediaUrl().isEmpty()) {
        if (!mpHost->getMediaLocation().isEmpty()) {
            url = QUrl::fromUserInput(mpHost->getMediaLocation());
        } else {
            url = QUrl::fromUserInput(QStringLiteral("https://www.%1/media/").arg(mpHost->mUrl));
        }
    } else {
        url = QUrl::fromUserInput(mediaData.getMediaUrl());
    }

    return url;
}

bool TMedia::isValidUrl(QUrl& url)
{
    bool isValid = false;

    if (!url.isValid()) {
        qWarning() << QStringLiteral("TMedia::validUrl() WARNING - Attempt made to reference an invalid URL: %1 and the error message was:\"%2\".")
            .arg(url.toString(), url.errorString());
    } else {
        isValid = true;
    }

    return isValid;
}

bool TMedia::isFileRelative(TMediaData& mediaData)
{
    bool isFileRelative = false;

    if (!QFileInfo(mediaData.getMediaFileName()).isRelative()) {
        qWarning() << QStringLiteral("TMedia::isFileRelative() WARNING - Attempt made to send an absolute path as a media file name: %1.  Only relative paths are permitted.")
            .arg(mediaData.getMediaFileName());
    } else {
        isFileRelative = true;
    }

    return isFileRelative;
}

QStringList TMedia::parseFileNameList(TMediaData& mediaData, QDir &dir)
{
    QStringList fileNameList;

    // No more than one '*' wildcard per the specification
    if ((mediaData.getMediaFileName().contains('*') || mediaData.getMediaFileName().contains('?')) && mediaData.getMediaFileName().count('*') < 2) {
        dir.setNameFilters(QStringList() << mediaData.getMediaFileName());
        QStringList fileNames(dir.entryList(QDir::Files | QDir::Readable, QDir::Name));

        for (auto& fileName : qAsConst(fileNames)) {
            if (!mediaData.getMediaTag().isEmpty()) {
                fileNameList << QStringLiteral("%1/%2/%3").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag(), fileName);
            } else {
                fileNameList << QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), fileName);
            }
        }
    } else {
        if (!mediaData.getMediaFileName().contains('.')) {
            switch (mediaData.getMediaType()) {
                case TMediaData::MediaTypeSound:
                    mediaData.setMediaFileName(mediaData.getMediaFileName().append(".wav"));
                    break;
                case TMediaData::MediaTypeMusic:
                    mediaData.setMediaFileName(mediaData.getMediaFileName().append(".mid"));
                    break;
            }
        }

        if (!mediaData.getMediaTag().isEmpty()) {
            fileNameList << QStringLiteral("%1/%2/%3")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag(), mediaData.getMediaFileName());
        } else {
            fileNameList << QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());
        }
    }

    return fileNameList;
}

QStringList TMedia::getFileNameList(TMediaData& mediaData)
{
    QStringList fileNameList;

    QString mediaPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << QStringLiteral("TMedia::getFileNameList() WARNING - Attempt made to create a directory failed: %1")
            .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return fileNameList;
    }

    if (!mediaData.getMediaTag().isEmpty()) {
        QString mediaTagPath = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag());
        QDir mediaTagDir(mediaTagPath);

        if (!mediaTagDir.mkpath(mediaTagPath)) {
            qWarning() << QStringLiteral("TMedia::getFileNameList() WARNING - Attempt made to create a directory failed: %1")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag());
            return fileNameList;
        }

        fileNameList = TMedia::parseFileNameList(mediaData, mediaTagDir);
    }

    // Enter this block if no mediaTag was specified.  Also, per the specification, if mediaTag was specified above, but we did not
    // find anything in a mediaTag directory, fall back and search for the mediaFileName in the root "media" directory.
    if (fileNameList.isEmpty()) {
        mediaData.setMediaTag(QString());
        fileNameList = TMedia::parseFileNameList(mediaData, mediaDir);
    }

    return fileNameList;
}

QUrl TMedia::getFileUrl(TMediaData& mediaData)
{
    QUrl fileUrl;

    if (!mpHost->getMediaLocation().isEmpty()) {
        bool endsWithSlash = mpHost->getMediaLocation().endsWith('/');

        if (!mediaData.getMediaTag().isEmpty()) {
            if (!endsWithSlash) {
                fileUrl = QUrl::fromUserInput(QStringLiteral("%1/%2/%3").arg(mpHost->getMediaLocation(), mediaData.getMediaTag(), mediaData.getMediaFileName()));
            } else {
                fileUrl = QUrl::fromUserInput(QStringLiteral("%1%2/%3").arg(mpHost->getMediaLocation(), mediaData.getMediaTag(), mediaData.getMediaFileName()));
            }
        } else {
            if (!endsWithSlash) {
                fileUrl = QUrl::fromUserInput(QStringLiteral("%1/%2").arg(mpHost->getMediaLocation(), mediaData.getMediaFileName()));
            } else {
                fileUrl = QUrl::fromUserInput(QStringLiteral("%1%2").arg(mpHost->getMediaLocation(), mediaData.getMediaFileName()));
            }
        }
    }

    return fileUrl;
}

void TMedia::writeFile(QNetworkReply* reply)
{
    TEvent event {};
    TMediaData mediaData = mMediaDownloads.value(reply);
    mMediaDownloads.remove(reply);

    if (reply->error() != QNetworkReply::NoError) {
        event.mArgumentList << QStringLiteral("sysDownloadError");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->errorString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

        reply->deleteLater();
        mpHost->raiseEvent(event);
    } else {
        QFile localFile(mediaData.getMediaAbsolutePathFileName());

        if (!localFile.open(QFile::WriteOnly)) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("failureToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("unableToOpenLocalFileForWriting");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

            reply->deleteLater();
            mpHost->raiseEvent(event);
        }

        qint64 bytesWritten = localFile.write(reply->readAll());

        if (bytesWritten == -1) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("failureToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("unableToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

            reply->deleteLater();
            mpHost->raiseEvent(event);
        } else {
            localFile.flush();

            if (localFile.error() == QFile::NoError) {
                event.mArgumentList << QLatin1String("sysDownloadDone");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QString::number(bytesWritten);
                event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;

                localFile.close();

                reply->deleteLater();
                mpHost->raiseEvent(event);

                switch (mediaData.getMediaType()) {
                    case TMediaData::MediaTypeSound:
                        TMedia::playSound(mediaData);
                        break;
                    case TMediaData::MediaTypeMusic:
                        TMedia::playMusic(mediaData);
                }
            } else {
                event.mArgumentList << QLatin1String("sysDownloadError");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << QLatin1String("failureToWriteLocalFile");
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << mediaData.getMediaAbsolutePathFileName();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                event.mArgumentList << localFile.errorString();
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

                reply->deleteLater();
                mpHost->raiseEvent(event);
            }
        }
    }
}

void TMedia::downloadFile(TMediaData& mediaData)
{
    QString mediaPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir mediaDir(mediaPath);

    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << QStringLiteral("TMedia::downloadFile() WARNING - Attempt made to create a directory failed: %1")
            .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return;
    }

    if (!mediaData.getMediaTag().isEmpty()) {
        QString mediaTagPath = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag());
        QDir mediaTagDir(mediaTagPath);

        if (!mediaTagDir.mkpath(mediaTagPath)) {
            qWarning() << QStringLiteral("TMedia::downloadFile() WARNING - Attempt made to create a directory failed: %1")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag());
            return;
        }
    }

    QDir dir;
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    if (!dir.mkpath(cacheDir)) {
        qWarning() << "TMedia::downloadFile() WARNING - Couldn't create cache directory for sound file(s): " << cacheDir;
        return;
    }

    QUrl fileUrl = getFileUrl(mediaData);

    if (!TMedia::isValidUrl(fileUrl)) {
        return;
    } else {
        auto diskCache = new QNetworkDiskCache(this);
        diskCache->setCacheDirectory(cacheDir);
        mpNetworkAccessManager->setCache(diskCache);

        QNetworkRequest request = QNetworkRequest(fileUrl);
        request.setRawHeader(QByteArray("User-Agent"), QByteArray(QStringLiteral("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, APP_BUILD).toUtf8().constData()));
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
#ifndef QT_NO_SSL
        if (fileUrl.scheme() == QStringLiteral("https")) {
            QSslConfiguration config(QSslConfiguration::defaultConfiguration());
            request.setSslConfiguration(config);
        }
#endif
        mpHost->updateProxySettings(mpNetworkAccessManager);
        QNetworkReply* getReply = mpNetworkAccessManager->get(request);
        mMediaDownloads.insert(getReply, mediaData);

        connect(getReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError) {
            qWarning() << "TMedia::downloadFile() WARNING - couldn't download sound from " << fileUrl.url();
            getReply->deleteLater();
        });
    }
}

QList<TMediaPlayer*> TMedia::getMediaPlayerList(TMediaData& mediaData)
{
    QList<TMediaPlayer*> mTMediaPlayerList;

    switch (mediaData.getMediaProtocol()) {
        case TMediaData::MediaProtocolMSP:
            switch (mediaData.getMediaType()) {
                case TMediaData::MediaTypeSound:
                    mTMediaPlayerList = mMSPSoundList;
                    break;
                case TMediaData::MediaTypeMusic:
                    mTMediaPlayerList = mMSPMusicList;
                    break;
            }
            break;

        case TMediaData::MediaProtocolGMCP:
            switch (mediaData.getMediaType()) {
                case TMediaData::MediaTypeSound:
                    mTMediaPlayerList = mGMCPSoundList;
                    break;
                case TMediaData::MediaTypeMusic:
                    mTMediaPlayerList = mGMCPMusicList;
                    break;
                case TMediaData::MediaTypeVideo:
                    mTMediaPlayerList = mGMCPVideoList;
                    break;
            }
            break;
    }

    return mTMediaPlayerList;
}

TMediaPlayer* TMedia::getMediaPlayer(TMediaData& mediaData)
{
    TMediaPlayer* pPlayer = nullptr;
    QList<TMediaPlayer*> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer*> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) { // Find first available inactive QMediaPlayer
        TMediaPlayer* pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer->getMediaPlayer()->state() != QMediaPlayer::PlayingState && pTestPlayer->getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            pPlayer = pTestPlayer;
            break;
        }
    }

    if (!pPlayer) { // No available QMediaPlayer, create a new one.
        pPlayer = new TMediaPlayer(this->mpHost, mediaData);

        if (!pPlayer) { // It should be impossible to ever reach this.
            qWarning() << QStringLiteral("TMedia::getMediaPlayer() WARNING - Unable to create new QMediaPlayer object.");
            return pPlayer;
        }

        switch (mediaData.getMediaProtocol()) {
            case TMediaData::MediaProtocolMSP:
                switch (mediaData.getMediaType()) {
                    case TMediaData::MediaTypeSound:
                        mMSPSoundList.append(pPlayer);
                        break;
                    case TMediaData::MediaTypeMusic:
                        mMSPMusicList.append(pPlayer);
                        break;
                }
                break;

            case TMediaData::MediaProtocolGMCP:
                switch (mediaData.getMediaType()) {
                    case TMediaData::MediaTypeSound:
                        mGMCPSoundList.append(pPlayer);
                        break;
                    case TMediaData::MediaTypeMusic:
                        mGMCPMusicList.append(pPlayer);
                        break;
                    case TMediaData::MediaTypeVideo:
                        mGMCPVideoList.append(pPlayer);
                        break;
                }
                break;
        }
    }

    disconnect(pPlayer->getMediaPlayer(), &QMediaPlayer::stateChanged, nullptr, nullptr);

    connect(pPlayer->getMediaPlayer(), &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            TEvent mediaFinished {};
            mediaFinished.mArgumentList.append("sysMediaFinished");
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mediaFinished.mArgumentList.append(pPlayer->getMediaPlayer()->media().canonicalUrl().fileName());
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mediaFinished.mArgumentList.append(pPlayer->getMediaPlayer()->media().canonicalUrl().path());
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

            if (mpHost) {
                // The host may have gone away if the sound was a long one
                // and we are multi-playing so we ought to test it...
                mpHost->raiseEvent(mediaFinished);
            }
        }
    });

    return pPlayer;
}

TMediaPlayer* TMedia::matchMediaPlayer(TMediaData& mediaData, QString absolutePathFileName)
{
    TMediaPlayer* pPlayer = nullptr;
    QList<TMediaPlayer*> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
    QListIterator<TMediaPlayer*> itTMediaPlayer(mTMediaPlayerList);

    while (itTMediaPlayer.hasNext()) {
        TMediaPlayer* pTestPlayer = itTMediaPlayer.next();

        if (pTestPlayer->getMediaPlayer()->state() == QMediaPlayer::PlayingState && pTestPlayer->getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (pTestPlayer->getMediaPlayer()->media().canonicalUrl().toString().endsWith(absolutePathFileName)) { // Is the same sound or music playing?
                pPlayer = pTestPlayer;
                pPlayer->setMediaData(mediaData);
                pPlayer->getMediaPlayer()->setVolume(mediaData.getMediaVolume());
                break;
            }
        }
    }

    return pPlayer;
}

bool TMedia::doesMediaHavePriorityToPlay(TMediaData& mediaData, QString absolutePathFileName)
{
    bool doesMediaHavePriorityToPlay = true;

    if (mediaData.getMediaPriority() != TMediaData::MediaPriorityNotSet) {
        int maxMediaPriority = 0;

        QList<TMediaPlayer*> mTMediaPlayerList = TMedia::getMediaPlayerList(mediaData);
        QListIterator<TMediaPlayer*> itTMediaPlayer(mTMediaPlayerList);

        while (itTMediaPlayer.hasNext()) { // Find the maximum priority of all playing sounds
            TMediaPlayer* pTestPlayer = itTMediaPlayer.next();

            if (pTestPlayer->getMediaPlayer()->state() == QMediaPlayer::PlayingState && pTestPlayer->getMediaPlayer()->mediaStatus() != QMediaPlayer::LoadingMedia) {
                if (!pTestPlayer->getMediaPlayer()->media().canonicalUrl().toString().endsWith(absolutePathFileName)) { // Is it a different sound or music than specified?
                    if (pTestPlayer->getMediaData().getMediaPriority() != TMediaData::MediaPriorityNotSet
                        && pTestPlayer->getMediaData().getMediaPriority() > maxMediaPriority) {
                        maxMediaPriority = pTestPlayer->getMediaData().getMediaPriority();
                    }
                }
            }
        }

        if (maxMediaPriority >= mediaData.getMediaPriority()) { // Our media has a lower priority
            doesMediaHavePriorityToPlay = false;
        } else {
            TMediaData stopMediaData;
            stopMediaData.setMediaProtocol(mediaData.getMediaProtocol());
            stopMediaData.setMediaType(mediaData.getMediaType());
            mpHost->mpMedia->stopMedia(stopMediaData); // If we have the highest priority, stop everything else.
        }
    }

    return doesMediaHavePriorityToPlay;
}

void TMedia::playSound(TMediaData& soundData)
{
    if (soundData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (soundData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

     // File wildcards "*" and "?" could return more than one sound so we process as QStringList.
    QStringList fileNameList = TMedia::getFileNameList(soundData);

    if (fileNameList.isEmpty()) { // This should not happen.
        qWarning() << QStringLiteral("TMedia::playSound() WARNING - Could not generate a list of media file names.");
        return;
    }

    TMediaPlayer* pPlayer = TMedia::getMediaPlayer(soundData);

    if (!pPlayer) { // It should be impossible to ever reach this.
        qWarning() << QStringLiteral("TMedia::playSound() WARNING - Unable to create new QMediaPlayer object.");
        return;
    }

    QString absolutePathFileName;

    if (soundData.getMediaLoops() == TMediaData::MediaLoopsDefault) { // Play once
        if (fileNameList.size() > 1) {
            absolutePathFileName = fileNameList.at(qrand() % fileNameList.size());
        } else {
            absolutePathFileName = fileNameList.at(0);
        }

        if (!TMedia::doesMediaHavePriorityToPlay(soundData, absolutePathFileName)) { // Filter out other sound
            return;
        }

        pPlayer->getMediaPlayer()->setMedia(QUrl::fromLocalFile(absolutePathFileName));
    } else {
        QMediaPlaylist* playlist = new QMediaPlaylist;

        if (soundData.getMediaLoops() == TMediaData::MediaLoopsRepeat) { // Repeat indefinitely
            if (fileNameList.size() > 1) {
                absolutePathFileName = fileNameList.at(qrand() % fileNameList.size());
            } else {
                absolutePathFileName = fileNameList.at(0);
            }

            if (!TMedia::doesMediaHavePriorityToPlay(soundData, absolutePathFileName)) { // Filter out other sound
                return;
            }

            playlist->addMedia(QUrl::fromLocalFile(absolutePathFileName));
            playlist->setPlaybackMode(QMediaPlaylist::Loop);
        } else {
            for (int k = 0; k < soundData.getMediaLoops(); k++) { // Repeat a finite number of times
                if (fileNameList.size() > 1) {
                    absolutePathFileName = fileNameList.at(qrand() % fileNameList.size());
                } else {
                    absolutePathFileName = fileNameList.at(0);
                }

                if (k < 1 && !TMedia::doesMediaHavePriorityToPlay(soundData, absolutePathFileName)) { // Filter out other sound
                    return;
                }

                playlist->addMedia(QUrl::fromLocalFile(absolutePathFileName));
            }
        }

        playlist->setCurrentIndex(1);
        pPlayer->getMediaPlayer()->setPlaylist(playlist);
    }

    // Set volume and play
    pPlayer->getMediaPlayer()->setVolume(soundData.getMediaVolume());
    pPlayer->getMediaPlayer()->play();
}

void TMedia::playMusic(TMediaData& musicData)
{
    if (musicData.getMediaProtocol() == TMediaData::MediaProtocolMSP && !mpHost->mEnableMSP) {
        return;
    }

    if (musicData.getMediaProtocol() == TMediaData::MediaProtocolGMCP && !mpHost->mAcceptServerMedia) {
        return;
    }

    QStringList fileNameList = TMedia::getFileNameList(musicData);

    if (fileNameList.isEmpty()) { // This should not happen.
        qWarning() << QStringLiteral("TMedia::playMusic() WARNING - Could not generate a list of media file names.");
        return;
    }

    TMediaPlayer* pPlayer = TMedia::matchMediaPlayer(musicData, fileNameList.at(0));

    if (pPlayer != nullptr) { // Same music is already playing
        if (musicData.getMusicContinue() == TMediaData::MusicContinueDefault && musicData.getMediaLoops() == TMediaData::MediaLoopsDefault) {
            return; // Continue playing it && Don't add more iterations of it
        }
    } else {
        pPlayer = TMedia::getMediaPlayer(musicData);
    }

    // It should be impossible to ever reach this.
    if (!pPlayer) {
        qWarning() << QStringLiteral("TMedia::playMusic() WARNING - Unable to create new QMediaPlayer object.");
        return;
    }

    QString absolutePathFileName;

    if (musicData.getMediaLoops() == TMediaData::MediaLoopsDefault) { // Play once
        if (fileNameList.size() > 1) {
            absolutePathFileName = fileNameList.at(qrand() % fileNameList.size());
        } else {
            absolutePathFileName = fileNameList.at(0);
        }

        if (!TMedia::doesMediaHavePriorityToPlay(musicData, absolutePathFileName)) { // Filter out other music
            return;
        }

        pPlayer->getMediaPlayer()->setMedia(QUrl::fromLocalFile(absolutePathFileName));
    } else {
        QMediaPlaylist* playlist = new QMediaPlaylist;

        if (musicData.getMediaLoops() == TMediaData::MediaLoopsRepeat) { // Repeat indefinitely
            if (fileNameList.size() > 1) {
                absolutePathFileName = fileNameList.at(qrand() % fileNameList.size());
            } else {
                absolutePathFileName = fileNameList.at(0);
            }

            if (!TMedia::doesMediaHavePriorityToPlay(musicData, absolutePathFileName)) { // Filter out other music
                return;
            }

            playlist->addMedia(QUrl::fromLocalFile(absolutePathFileName));
            playlist->setPlaybackMode(QMediaPlaylist::Loop);
        } else {
            if (musicData.getMusicContinue() == TMediaData::MusicContinueDefault) {
                musicData.setMediaLoops(musicData.getMediaLoops() - 1); // Subtract the currently playing music from the total
            }

            for (int k = 0; k < musicData.getMediaLoops(); k++) { // Repeat a finite number of times
                if (fileNameList.size() > 1) {
                    absolutePathFileName = fileNameList.at(qrand() % fileNameList.size());
                } else {
                    absolutePathFileName = fileNameList.at(0);
                }

                if (k < 1 && !TMedia::doesMediaHavePriorityToPlay(musicData, absolutePathFileName)) { // Filter out other music, once
                    return;
                }

                playlist->addMedia(QUrl::fromLocalFile(absolutePathFileName));
            }
        }

        playlist->setCurrentIndex(1);
        pPlayer->getMediaPlayer()->setPlaylist(playlist);
    }

    // Set volume and play music
    pPlayer->getMediaPlayer()->setVolume(musicData.getMediaVolume());
    pPlayer->getMediaPlayer()->play();
}

TMediaData::MediaType TMedia::parseJSONByMediaType(QJsonObject& json)
{
    TMediaData::MediaType mediaType = TMediaData::MediaTypeNotSet;

    auto mediaTypeJSON = json.value(QStringLiteral("type"));

    if (mediaTypeJSON != QJsonValue::Undefined && !mediaTypeJSON.toString().isEmpty()) {
        if (mediaTypeJSON.toString().toLower() == "sound") {
            mediaType = TMediaData::MediaTypeSound;
        } else if (mediaTypeJSON.toString().toLower() == "music") {
            mediaType = TMediaData::MediaTypeMusic;
        } else if (mediaTypeJSON.toString().toLower() == "video") {
            mediaType = TMediaData::MediaTypeVideo;
        }
    }

    return mediaType;
}

QString TMedia::parseJSONByMediaFileName(QJsonObject& json)
{
    QString mediaFileName = QString();

    auto mediaFileNameJSON = json.value(QStringLiteral("name"));

    if (mediaFileNameJSON != QJsonValue::Undefined && !mediaFileNameJSON.toString().isEmpty()) {
        mediaFileName = mediaFileNameJSON.toString();
    }

    return mediaFileName;
}

int TMedia::parseJSONByMediaVolume(QJsonObject& json)
{
    int mediaVolume = TMediaData::MediaVolumeDefault;

    auto mediaVolumeJSON = json.value(QStringLiteral("volume"));

    if (mediaVolumeJSON != QJsonValue::Undefined && mediaVolumeJSON.isString() && !mediaVolumeJSON.toString().isEmpty()) {
        mediaVolume = mediaVolumeJSON.toString().toInt();

        if (mediaVolume == TMediaData::MediaVolumePreload) {
            {} // Volume of 0 supports preloading
        } else if (mediaVolume > TMediaData::MediaVolumeMax) {
            mediaVolume = TMediaData::MediaVolumeMax;
        } else if (mediaVolume < TMediaData::MediaVolumeMin) {
              mediaVolume = TMediaData::MediaVolumeMin;
        }
    } else if (mediaVolumeJSON != QJsonValue::Undefined && mediaVolumeJSON.toInt()) {
        mediaVolume = mediaVolumeJSON.toInt();

        if (mediaVolume == TMediaData::MediaVolumePreload) {
            {} // Volume of 0 supports preloading
        } else if (mediaVolume > TMediaData::MediaVolumeMax) {
            mediaVolume = TMediaData::MediaVolumeMax;
        } else if (mediaVolume < TMediaData::MediaVolumeMin) {
            mediaVolume = TMediaData::MediaVolumeMin;
        }
    }

    return mediaVolume;
}

int TMedia::parseJSONByMediaPriority(QJsonObject& json)
{
    int mediaPriority = TMediaData::MediaPriorityNotSet;

    auto mediaPriorityJSON = json.value(QStringLiteral("priority"));

    if (mediaPriorityJSON != QJsonValue::Undefined && mediaPriorityJSON.isString() && !mediaPriorityJSON.toString().isEmpty()) {
        mediaPriority = mediaPriorityJSON.toString().toInt();

        if (mediaPriority > TMediaData::MediaPriorityMax) {
            mediaPriority = TMediaData::MediaPriorityMax;
        } else if (mediaPriority < TMediaData::MediaPriorityMin) {
            mediaPriority = TMediaData::MediaPriorityMin;
        }
    } else if (mediaPriorityJSON != QJsonValue::Undefined && mediaPriorityJSON.toInt()) {
        mediaPriority = mediaPriorityJSON.toInt();

        if (mediaPriority > TMediaData::MediaPriorityMax) {
            mediaPriority = TMediaData::MediaPriorityMax;
        } else if (mediaPriority < TMediaData::MediaPriorityMin) {
            mediaPriority = TMediaData::MediaPriorityMin;
        }
    }

    return mediaPriority;
}

int TMedia::parseJSONByMediaLoops(QJsonObject& json)
{
    int mediaLoops = TMediaData::MediaLoopsDefault;

    auto mediaLoopsJSON = json.value(QStringLiteral("loops"));

    if (mediaLoopsJSON != QJsonValue::Undefined && mediaLoopsJSON.isString() && !mediaLoopsJSON.toString().isEmpty()) {
        mediaLoops = mediaLoopsJSON.toString().toInt();

        if (mediaLoops < TMediaData::MediaLoopsRepeat || mediaLoops == 0) {
            mediaLoops = TMediaData::MediaLoopsDefault;
        }
    } else if (mediaLoopsJSON != QJsonValue::Undefined && mediaLoopsJSON.toInt()) {
        mediaLoops = mediaLoopsJSON.toInt();

        if (mediaLoops < TMediaData::MediaLoopsRepeat || mediaLoops == 0) {
            mediaLoops = TMediaData::MediaLoopsDefault;
        }
    }

    return mediaLoops;
}

TMediaData::MusicContinue TMedia::parseJSONByMusicContinue(QJsonObject& json)
{
    TMediaData::MusicContinue musicContinue = TMediaData::MusicContinueDefault;

    auto musicContinueJSON = json.value(QStringLiteral("continue"));

    if (musicContinueJSON != QJsonValue::Undefined && musicContinueJSON.isString() && !musicContinueJSON.toString().isEmpty()) {
        if (musicContinueJSON.toString() == "false") {
            musicContinue = TMediaData::MusicContinueRestart;
        } else {
            musicContinue = TMediaData::MusicContinueDefault;
        }
    } else if (musicContinueJSON != QJsonValue::Undefined && musicContinueJSON.toBool(true) == false) {
        musicContinue = TMediaData::MusicContinueRestart;
    }

    return musicContinue;
}

QString TMedia::parseJSONByMediaTag(QJsonObject& json)
{
    QString mediaTag = QString();

    auto mediaTagJSON = json.value(QStringLiteral("tag"));

    if (mediaTagJSON != QJsonValue::Undefined && !mediaTagJSON.toString().isEmpty()) {
        mediaTag = mediaTagJSON.toString().toLower(); // To provide case insensitivity of MSP specification
    }

    return mediaTag;
}

QString TMedia::parseJSONByMediaTarget(QJsonObject& json)
{
    QString mediaTarget = QString();

    auto mediaTargetJSON = json.value(QStringLiteral("target"));

    if (mediaTargetJSON != QJsonValue::Undefined && !mediaTargetJSON.toString().isEmpty()) {
        mediaTarget = mediaTargetJSON.toString();
    }

    return mediaTarget;
}

QString TMedia::parseJSONByMediaUrl(QJsonObject& json)
{
    QString mediaUrl = QString();

    auto mediaUrlJSON = json.value(QStringLiteral("url"));

    if (mediaUrlJSON != QJsonValue::Undefined && !mediaUrlJSON.toString().isEmpty()) {
        mediaUrl = mediaUrlJSON.toString();
    }

    return mediaUrl;
}

QString TMedia::parseJSONByMediaKey(QJsonObject& json)
{
    QString mediaKey = QString();

    auto mediaKeyJSON = json.value(QStringLiteral("key"));

    if (mediaKeyJSON != QJsonValue::Undefined && !mediaKeyJSON.toString().isEmpty()) {
        mediaKey = mediaKeyJSON.toString();
    }

    return mediaKey;
}

void TMedia::parseJSONForMediaLoad(QJsonObject& json)
{
    TMediaData mediaData;

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json)); //Required
    mediaData.setMediaUrl(TMedia::parseJSONByMediaUrl(json)); //Required
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    mediaData.setMediaFileName(mediaData.getMediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/')));

    if (!TMedia::isFileRelative(mediaData) || mediaData.getMediaTag().contains("..")) { // Security
        return;
    }

    QUrl url = TMedia::parseUrl(mediaData);

    if (!TMedia::isValidUrl(url)) {
        return;
    } else if (mpHost->getMediaLocation().isEmpty() || url.toString() != mpHost->getMediaLocation()) {
        mpHost->setMediaLocation(url.toString());
    }

    QString absolutePathFileName;

    if (!mediaData.getMediaTag().isEmpty()) {
        absolutePathFileName = QStringLiteral("%1/%2/%3")
            .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaTag(), mediaData.getMediaFileName());
    } else {
        absolutePathFileName = QStringLiteral("%1/%2")
            .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());
    }

    mediaData.setMediaAbsolutePathFileName(absolutePathFileName);

    QFile mediaFile(absolutePathFileName);

    if (!mediaFile.exists()) {
        TMedia::downloadFile(mediaData);
    }
}

void TMedia::parseJSONForMediaPlay(QJsonObject& json)
{
    TMediaData mediaData;

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaType(TMedia::parseJSONByMediaType(json));

    if (mediaData.getMediaType() == TMediaData::MediaTypeNotSet) {
        mediaData.setMediaType(TMediaData::MediaTypeSound);
    }

    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json)); //Required
    mediaData.setMediaUrl(TMedia::parseJSONByMediaUrl(json));
    mediaData.setMediaKey(TMedia::parseJSONByMediaKey(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaTarget(TMedia::parseJSONByMediaTarget(json));
    mediaData.setMediaVolume(TMedia::parseJSONByMediaVolume(json));
    mediaData.setMediaLoops(TMedia::parseJSONByMediaLoops(json));
    mediaData.setMediaPriority(TMedia::parseJSONByMediaPriority(json));

    TMedia::playMedia(mediaData);
}

void TMedia::parseJSONForMediaStop(QJsonObject& json)
{
    TMediaData mediaData;

    mediaData.setMediaProtocol(TMediaData::MediaProtocolGMCP);
    mediaData.setMediaType(TMedia::parseJSONByMediaType(json));
    mediaData.setMediaFileName(TMedia::parseJSONByMediaFileName(json));
    mediaData.setMediaKey(TMedia::parseJSONByMediaKey(json));
    mediaData.setMediaTag(TMedia::parseJSONByMediaTag(json));
    mediaData.setMediaTarget(TMedia::parseJSONByMediaTarget(json));
    mediaData.setMediaPriority(TMedia::parseJSONByMediaPriority(json));

    TMedia::stopMedia(mediaData);
}
// End Private
