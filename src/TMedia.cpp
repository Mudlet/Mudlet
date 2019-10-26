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

TMedia::~TMedia()
{
	TMedia::stopSound();
	TMedia::stopMusic();
}

void TMedia::playMedia(TMediaData& mediaData)
{
    mediaData.getMediaFileName().replace(QLatin1Char('\\'), QLatin1Char('/'));

    if (!TMedia::isFileRelative(mediaData)) {
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
            switch (mediaData.getMediaCategory()) {
                case TMediaData::MediaCategorySound:
                    mediaData.getMediaFileName().append(".wav");
                    break;
                case TMediaData::MediaCategoryMusic:
                    mediaData.getMediaFileName().append(".mid");
                    break;
            }
        }

        if (!mediaData.getMediaType().isEmpty()) {
            absolutePathFileName = QStringLiteral("%1/%2/%3")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType(), mediaData.getMediaFileName());
        } else {
            absolutePathFileName = QStringLiteral("%1/%2")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());
        }

        QFile soundFile(absolutePathFileName);

        if (!soundFile.exists()) {
        	mediaData.setMediaAbsolutePathFileName(absolutePathFileName);
            TMedia::downloadFile(mediaData);
            return;
        }
    }

    switch (mediaData.getMediaCategory()) {
        case TMediaData::MediaCategorySound:
            TMedia::playSound(mediaData);
            break;
        case TMediaData::MediaCategoryMusic:
            TMedia::playMusic(mediaData);
            break;
    }
}

void TMedia::stopMedia(TMediaData::MediaCategory mediaCategory)
{
    switch (mediaCategory) {
        case TMediaData::MediaCategorySound:
            TMedia::stopSound();
            break;
        case TMediaData::MediaCategoryMusic:
            TMedia::stopMusic();
            break;
    }
}
// End Public

// Private
QUrl TMedia::parseUrl(TMediaData& mediaData)
{
    QUrl url;

    if (mediaData.getMediaFileName() == "Off") {
        if (mediaData.getMediaUrl().isEmpty()) { // MSP is !!SOUND(Off) or !!MUSIC(Off)
            switch (mediaData.getMediaCategory()) {
                case TMediaData::MediaCategorySound:
                    TMedia::stopSound();
                    break;
                case TMediaData::MediaCategoryMusic:
                    TMedia::stopMusic();
                    break;
            }
        } else { // MSP is !!SOUND(Off U=https://example.com/sounds) or !!MUSIC(Off U=https://example.com/sounds)
            url = QUrl::fromUserInput(mediaData.getMediaUrl());
        }
    } else if (mediaData.getMediaUrl().isEmpty()) {
        if (!mpHost->getMediaLocation().isEmpty()) {
            url = QUrl::fromUserInput(mpHost->getMediaLocation());
        } else {
            url = QUrl::fromUserInput(QStringLiteral("https://www.%1/sounds/").arg(mpHost->mUrl));
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
            if (!mediaData.getMediaType().isEmpty()) {
                fileNameList << QStringLiteral("%1/%2/%3").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType(), fileName);
            } else {
                fileNameList << QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), fileName);
            }
        }
    } else {
        if (!mediaData.getMediaType().isEmpty()) {
            fileNameList << QStringLiteral("%1/%2/%3")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType(), mediaData.getMediaFileName());
        } else {
            fileNameList << QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaFileName());
        }
    }

    return fileNameList;
}

QStringList TMedia::getFileNameList(TMediaData& mediaData)
{
    QStringList fileNameList;

    QString soundsPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir soundDir(soundsPath);

    if (!soundDir.mkpath(soundsPath)) {
        qWarning() << QStringLiteral("TMedia::getFileNameList() WARNING - Attempt made to create a directory failed: %1")
            .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return fileNameList;
    }

    if (!mediaData.getMediaType().isEmpty()) {
        QString soundTypePath = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType());
        QDir soundTypeDir(soundTypePath);

        if (!soundTypeDir.mkpath(soundTypePath)) {
            qWarning() << QStringLiteral("TMedia::getFileNameList() WARNING - Attempt made to create a directory failed: %1")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType());
            return fileNameList;
        }

        fileNameList = TMedia::parseFileNameList(mediaData, soundTypeDir);
    }

    // Enter this block if no mediaType was specified.  Also, per the specification, if mediaType was specified above, but we did not
    // find anything in a mediaType directory, fall back and search for the mediaFileName in the root "media" directory.
    if (fileNameList.isEmpty()) {
    	mediaData.setMediaType(QString());
        fileNameList = TMedia::parseFileNameList(mediaData, soundDir);
    }

    return fileNameList;
}

QUrl TMedia::getFileUrl(TMediaData& mediaData)
{
    QUrl fileUrl;

    if (!mpHost->getMediaLocation().isEmpty()) {
        bool endsWithSlash = mpHost->getMediaLocation().endsWith('/');

        if (!mediaData.getMediaType().isEmpty()) {
            if (!endsWithSlash) {
                fileUrl = QUrl::fromUserInput(QStringLiteral("%1/%2/%3").arg(mpHost->getMediaLocation(), mediaData.getMediaType(), mediaData.getMediaFileName()));
            } else {
                fileUrl = QUrl::fromUserInput(QStringLiteral("%1%2/%3").arg(mpHost->getMediaLocation(), mediaData.getMediaType(), mediaData.getMediaFileName()));
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

                switch (mediaData.getMediaCategory()) {
                    case TMediaData::MediaCategorySound:
                        TMedia::playSound(mediaData);
                        break;
                    case TMediaData::MediaCategoryMusic:
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
    QString soundsPath = mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName());
    QDir soundDir(soundsPath);

    if (!soundDir.mkpath(soundsPath)) {
        qWarning() << QStringLiteral("TMedia::downloadFile() WARNING - Attempt made to create a directory failed: %1")
            .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()));
        return;
    }

    if (!mediaData.getMediaType().isEmpty()) {
        QString soundTypePath = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType());
        QDir soundTypeDir(soundTypePath);

        if (!soundTypeDir.mkpath(soundTypePath)) {
            qWarning() << QStringLiteral("TMedia::downloadFile() WARNING - Attempt made to create a directory failed: %1")
                .arg(mudlet::getMudletPath(mudlet::profileMediaPath, mpHost->getName()), mediaData.getMediaType());
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

QMediaPlayer* TMedia::getMediaPlayer(TMediaData& mediaData)
{
    QMediaPlayer* pPlayer = nullptr;
    QList<QMediaPlayer*> mQMediaPlayerList;

    switch (mediaData.getMediaCategory()) {
        case TMediaData::MediaCategorySound:
            mQMediaPlayerList = mSoundList;
            break;
        case TMediaData::MediaCategoryMusic:
            mQMediaPlayerList = mMusicList;
            break;
    }

    QListIterator<QMediaPlayer*> itQMediaPlayer(mQMediaPlayerList);

    while (itQMediaPlayer.hasNext()) { // Find first available inactive QMediaPlayer
        QMediaPlayer* pTestPlayer = itQMediaPlayer.next();

        if (pTestPlayer->state() != QMediaPlayer::PlayingState && pTestPlayer->mediaStatus() != QMediaPlayer::LoadingMedia) {
            pPlayer = pTestPlayer;
            break;
        }
    }

    if (!pPlayer) { // No available QMediaPlayer, create a new one.
        pPlayer = new QMediaPlayer(this);

        if (!pPlayer) { // It should be impossible to ever reach this.
            qWarning() << QStringLiteral("TMedia::getMediaPlayer() WARNING - Unable to create new QMediaPlayer object.");
            return pPlayer;
        }

        switch (mediaData.getMediaCategory()) {
            case TMediaData::MediaCategorySound:
                mSoundList.append(pPlayer);
                break;
            case TMediaData::MediaCategoryMusic:
                mMusicList.append(pPlayer);
                break;
        }
    }

    disconnect(pPlayer, &QMediaPlayer::stateChanged, nullptr, nullptr);

    connect(pPlayer, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            TEvent mediaFinished {};
            mediaFinished.mArgumentList.append("sysMediaFinished");
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mediaFinished.mArgumentList.append(pPlayer->media().canonicalUrl().fileName());
            mediaFinished.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mediaFinished.mArgumentList.append(pPlayer->media().canonicalUrl().path());
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

QMediaPlayer* TMedia::matchMediaPlayer(TMediaData& mediaData, QString absolutePathFileName)
{
    QMediaPlayer* pPlayer = nullptr;
    QList<QMediaPlayer*> mQMediaPlayerList;

    switch (mediaData.getMediaCategory()) {
        case TMediaData::MediaCategorySound:
            mQMediaPlayerList = mSoundList;
            break;
        case TMediaData::MediaCategoryMusic:
            mQMediaPlayerList = mMusicList;
            break;
    }

    QListIterator<QMediaPlayer*> itQMediaPlayer(mQMediaPlayerList);

    while (itQMediaPlayer.hasNext()) {
        QMediaPlayer* pTestPlayer = itQMediaPlayer.next();

        if (pTestPlayer->state() == QMediaPlayer::PlayingState && pTestPlayer->mediaStatus() != QMediaPlayer::LoadingMedia) {
            if (pTestPlayer->media().canonicalUrl().toString().endsWith(absolutePathFileName)) { // Is the same sound or music playing?
                pPlayer = pTestPlayer;
                break;
            }
        }
    }

    return pPlayer;
}

void TMedia::playSound(TMediaData& soundData)
{
     // File wildcards "*" and "?" could return more than one sound so we process as QStringList.
    QStringList fileNameList = TMedia::getFileNameList(soundData);

    if (fileNameList.isEmpty()) { // This should not happen.
        qWarning() << QStringLiteral("TMedia::playSound() WARNING - Could not generate a list of sound file names.");
        return;
    }

    QMediaPlayer* pPlayer = TMedia::getMediaPlayer(soundData);

    if (!pPlayer) { // It should be impossible to ever reach this.
        qWarning() << QStringLiteral("TMedia::playSound() WARNING - Unable to create new QMediaPlayer object.");
        return;
    }

    if (soundData.getMediaLength() == TMediaData::MediaLengthDefault) { // Play once
        if (fileNameList.size() > 1) {
            pPlayer->setMedia(QUrl::fromLocalFile(fileNameList.at(qrand() % fileNameList.size())));
        } else {
            pPlayer->setMedia(QUrl::fromLocalFile(fileNameList.at(0)));
        }
    } else {
        QMediaPlaylist* playlist = new QMediaPlaylist;

        if (soundData.getMediaLength() == TMediaData::MediaLengthRepeat) { // Repeat indefinitely
            if (fileNameList.size() > 1) {
                playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(qrand() % fileNameList.size())));
            } else {
                playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(0)));
            }
            playlist->setPlaybackMode(QMediaPlaylist::Loop);
        } else {
            for (int k = 0; k < soundData.getMediaLength(); k++) { // Repeat a finite number of times
                if (fileNameList.size() > 1) {
                    playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(qrand() % fileNameList.size())));
                } else {
                    playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(0)));
                }
            }
        }

        playlist->setCurrentIndex(1);
        pPlayer->setPlaylist(playlist);
    }

    // Set volume and play
    pPlayer->setVolume(soundData.getMediaVolume());
    pPlayer->play();
}

void TMedia::stopSound()
{
    QListIterator<QMediaPlayer*> itQMediaPlayer(mSoundList);

    while (itQMediaPlayer.hasNext()) {
        QMediaPlayer* pPlayer = itQMediaPlayer.next();
        pPlayer->stop();
    }
}

void TMedia::playMusic(TMediaData& musicData)
{
    QStringList fileNameList = TMedia::getFileNameList(musicData);

    if (fileNameList.isEmpty()) { // This should not happen.
        qWarning() << QStringLiteral("TMedia::playMusic() WARNING - Could not generate a list of sound file names.");
        return;
    }

    QMediaPlayer* pPlayer = TMedia::matchMediaPlayer(musicData, fileNameList.at(0));

    if (pPlayer != nullptr) { // Same music is already playing
        if (musicData.getMusicContinue() == TMediaData::MediaContinueDefault && musicData.getMediaLength() == TMediaData::MediaLengthDefault) {
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

    if (musicData.getMediaLength() == TMediaData::MediaLengthDefault) { // Play once
        if (fileNameList.size() > 1) {
            pPlayer->setMedia(QUrl::fromLocalFile(fileNameList.at(qrand() % fileNameList.size())));
        } else {
            pPlayer->setMedia(QUrl::fromLocalFile(fileNameList.at(0)));
        }
    } else {
        QMediaPlaylist* playlist = new QMediaPlaylist;

        if (musicData.getMediaLength() == TMediaData::MediaLengthRepeat) { // Repeat indefinitely
            if (fileNameList.size() > 1) {
                playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(qrand() % fileNameList.size())));
            } else {
                playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(0)));
            }
            playlist->setPlaybackMode(QMediaPlaylist::Loop);
        } else {
            if (musicData.getMusicContinue() == TMediaData::MediaContinueDefault) {
                musicData.setMediaLength(musicData.getMediaLength() - 1); // Subtract the currently playing music from the total
            }

            for (int k = 0; k < musicData.getMediaLength(); k++) { // Repeat a finite number of times
                if (fileNameList.size() > 1) {
                    playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(qrand() % fileNameList.size())));
                } else {
                    playlist->addMedia(QUrl::fromLocalFile(fileNameList.at(0)));
                }
            }
        }

        playlist->setCurrentIndex(1);
        pPlayer->setPlaylist(playlist);
    }

    // Set volume and play music
    pPlayer->setVolume(musicData.getMediaVolume());
    pPlayer->play();
}

void TMedia::stopMusic() {
    QListIterator<QMediaPlayer*> itQMediaPlayer(mMusicList);

    while (itQMediaPlayer.hasNext()) {
        QMediaPlayer* pPlayer = itQMediaPlayer.next();
        pPlayer->stop();
    }
}
// End Private