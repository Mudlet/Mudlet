/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TSpatialAudio.h"
#include "Host.h"
#include "mudlet.h"
#include "ctelnet.h"
#include <QtMath>
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDataStream>
#include <QMediaFormat>
#include <QAudioDecoder>

// ============================================================================
// TSpatialAudioSource implementation
// ============================================================================

TSpatialAudioSource::TSpatialAudioSource(QAudioEngine* engine, const QString& name)
    : mName(name)
    , mAudioEngine(engine)
{
    mSpatialSound = new QSpatialSound(engine);
}

TSpatialAudioSource::~TSpatialAudioSource()
{
    if (mSpatialSound) {
        mSpatialSound->stop();
        delete mSpatialSound;
        mSpatialSound = nullptr;
    }
}

void TSpatialAudioSource::play()
{
    if (mSpatialSound) {
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudioSource::play() - Playing:" << mName << "source:" << mSource;
        qDebug() << "TSpatialAudioSource::play() - Current volume:" << mSpatialSound->volume();
        qDebug() << "TSpatialAudioSource::play() - Current loops:" << mSpatialSound->loops();
        qDebug() << "TSpatialAudioSource::play() - Source URL:" << mSpatialSound->source();
#endif
        mSpatialSound->play();
        mIsPlaying = true;
        mIsPaused = false;
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudioSource::play() - QSpatialSound play() called";
#endif
    } else {
        qWarning() << "TSpatialAudioSource::play() - No QSpatialSound object for:" << mName;
    }
}

void TSpatialAudioSource::pause()
{
    if (mSpatialSound) {
        mSpatialSound->pause();
        mIsPlaying = false;
        mIsPaused = true;
    }
}

void TSpatialAudioSource::stop()
{
    if (mSpatialSound) {
        mSpatialSound->stop();
        mIsPlaying = false;
        mIsPaused = false;
    }
}

void TSpatialAudioSource::setSource(const QString& filePath)
{
    if (mSpatialSound) {
        mSource = filePath;
        QUrl url = QUrl::fromLocalFile(filePath);
        if (!url.isLocalFile() && filePath.startsWith(qsl("http"))) {
            url = QUrl(filePath);
        }
        mSpatialSound->setSource(url);
    }
}

void TSpatialAudioSource::setPosition(float azimuth, float elevation, float distance)
{
    mAzimuth = qBound(-180.0f, azimuth, 180.0f);
    mElevation = qBound(-90.0f, elevation, 90.0f);
    mDistance = qMax(0.0f, distance);
    updateCartesianPosition();
}

void TSpatialAudioSource::setAzimuth(float degrees)
{
    mAzimuth = qBound(-180.0f, degrees, 180.0f);
    updateCartesianPosition();
}

void TSpatialAudioSource::setElevation(float degrees)
{
    mElevation = qBound(-90.0f, degrees, 90.0f);
    updateCartesianPosition();
}

void TSpatialAudioSource::setDistance(float meters)
{
    mDistance = qMax(0.0f, meters);
    updateCartesianPosition();
}

void TSpatialAudioSource::updateCartesianPosition()
{
    if (!mSpatialSound) {
        return;
    }
    
    // Convert spherical to Cartesian coordinates
    // Azimuth: 0 = front, 90 = right, -90 = left, 180/-180 = behind
    // Elevation: 0 = level, 90 = above, -90 = below
    const float azimuthRad = qDegreesToRadians(mAzimuth);
    const float elevationRad = qDegreesToRadians(mElevation);
    
    const float x = mDistance * qSin(azimuthRad) * qCos(elevationRad);
    const float y = mDistance * qSin(elevationRad);
    const float z = -mDistance * qCos(azimuthRad) * qCos(elevationRad);
    
    mSpatialSound->setPosition(QVector3D(x, y, z));
}

void TSpatialAudioSource::setVolume(float volume)
{
    if (mSpatialSound) {
        mSpatialSound->setVolume(qBound(0.0f, volume, 1.0f));
    }
}

float TSpatialAudioSource::volume() const
{
    return mSpatialSound ? mSpatialSound->volume() : 0.0f;
}

void TSpatialAudioSource::setOcclusion(float amount)
{
    if (mSpatialSound) {
        mSpatialSound->setOcclusionIntensity(qBound(0.0f, amount, 4.0f));
    }
}

float TSpatialAudioSource::occlusion() const
{
    return mSpatialSound ? mSpatialSound->occlusionIntensity() : 0.0f;
}

void TSpatialAudioSource::setSize(float size)
{
    if (mSpatialSound) {
        mSpatialSound->setSize(qMax(0.0f, size));
    }
}

float TSpatialAudioSource::size() const
{
    return mSpatialSound ? mSpatialSound->size() : 0.0f;
}

void TSpatialAudioSource::setLoops(int count)
{
    if (mSpatialSound) {
        if (count < 0) {
            mSpatialSound->setLoops(QSpatialSound::Infinite);
        } else {
            mSpatialSound->setLoops(count);
        }
    }
}

int TSpatialAudioSource::loops() const
{
    return mSpatialSound ? mSpatialSound->loops() : 0;
}

// ============================================================================
// TSpatialAudioRoom implementation
// ============================================================================

TSpatialAudioRoom::TSpatialAudioRoom(QAudioEngine* engine)
    : mAudioEngine(engine)
{
    mAudioRoom = new QAudioRoom(engine);
    
    // Set reasonable defaults
    mAudioRoom->setDimensions(QVector3D(10.0f, 10.0f, 4.0f));
    mAudioRoom->setReflectionGain(0.5f);
    mAudioRoom->setReverbGain(0.3f);
    mAudioRoom->setReverbTime(1.0f);
    mAudioRoom->setReverbBrightness(0.0f);
}

TSpatialAudioRoom::~TSpatialAudioRoom()
{
    if (mAudioRoom) {
        delete mAudioRoom;
        mAudioRoom = nullptr;
    }
}

void TSpatialAudioRoom::setDimensions(float width, float height, float depth)
{
    if (mAudioRoom) {
        mAudioRoom->setDimensions(QVector3D(width, height, depth));
    }
}

void TSpatialAudioRoom::setDimensions(const QVector3D& dimensions)
{
    if (mAudioRoom) {
        mAudioRoom->setDimensions(dimensions);
    }
}

QVector3D TSpatialAudioRoom::dimensions() const
{
    return mAudioRoom ? mAudioRoom->dimensions() : QVector3D();
}

void TSpatialAudioRoom::setWallMaterial(QAudioRoom::Wall wall, QAudioRoom::Material material)
{
    if (mAudioRoom) {
        mAudioRoom->setWallMaterial(wall, material);
    }
}

void TSpatialAudioRoom::setReflectionGain(float gain)
{
    if (mAudioRoom) {
        mAudioRoom->setReflectionGain(qBound(0.0f, gain, 5.0f));
    }
}

float TSpatialAudioRoom::reflectionGain() const
{
    return mAudioRoom ? mAudioRoom->reflectionGain() : 0.0f;
}

void TSpatialAudioRoom::setReverbGain(float gain)
{
    if (mAudioRoom) {
        mAudioRoom->setReverbGain(qBound(0.0f, gain, 5.0f));
    }
}

float TSpatialAudioRoom::reverbGain() const
{
    return mAudioRoom ? mAudioRoom->reverbGain() : 0.0f;
}

void TSpatialAudioRoom::setReverbTime(float seconds)
{
    if (mAudioRoom) {
        mAudioRoom->setReverbTime(qBound(0.0f, seconds, 20.0f));
    }
}

float TSpatialAudioRoom::reverbTime() const
{
    return mAudioRoom ? mAudioRoom->reverbTime() : 0.0f;
}

void TSpatialAudioRoom::setReverbBrightness(float brightness)
{
    if (mAudioRoom) {
        mAudioRoom->setReverbBrightness(qBound(-1.0f, brightness, 1.0f));
    }
}

float TSpatialAudioRoom::reverbBrightness() const
{
    return mAudioRoom ? mAudioRoom->reverbBrightness() : 0.0f;
}

// ============================================================================
// TSpatialAudio implementation
// ============================================================================

TSpatialAudio::TSpatialAudio(Host* host)
    : QObject(host)
    , mpHost(host)
{
    mNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mNetworkAccessManager, &QNetworkAccessManager::finished, this, &TSpatialAudio::downloadFinished);
}

TSpatialAudio::~TSpatialAudio()
{
    // Following TMedia's pattern: let Qt's parent-child cleanup handle destruction order
    // But explicitly clear sources first to avoid dangling pointers
    mAPISpatialSources.clear();
    mGMCPSpatialSources.clear();
}

bool TSpatialAudio::initialize()
{
    if (mAudioEngine) {
        return true;  // Already initialized
    }
    
    mAudioEngine = new QAudioEngine(this);
    mAudioEngine->setOutputMode(QAudioEngine::Headphone);  // Default to headphone mode
    
    mAudioListener = new QAudioListener(mAudioEngine);
    mAudioListener->setPosition(QVector3D(0, 0, 0));
    mAudioListener->setRotation(QQuaternion());
    
    mAudioEngine->start();
    
    qDebug() << "TSpatialAudio: Initialized spatial audio engine";
    return true;
}

void TSpatialAudio::shutdown()
{
    if (!mAudioEngine) {
        return;
    }
    
    // Stop all sources first
    stopAllSources(ProtocolAPI);
    stopAllSources(ProtocolGMCP);
    
    // Clear source containers
    mAPISpatialSources.clear();
    mGMCPSpatialSources.clear();
    
    // Remove room
    removeRoom();
    
    // Stop engine and let Qt parent-child cleanup handle deletion
    if (mAudioEngine) {
        mAudioEngine->stop();
        mAudioEngine->deleteLater();  // Use Qt's delayed deletion
        mAudioEngine = nullptr;
        mAudioListener = nullptr;  // Will be deleted with engine
    }
    
    qDebug() << "TSpatialAudio: Shut down spatial audio engine";
}

void TSpatialAudio::setOutputMode(QAudioEngine::OutputMode mode)
{
    if (mAudioEngine) {
        mAudioEngine->setOutputMode(mode);
    }
}

QAudioEngine::OutputMode TSpatialAudio::outputMode() const
{
    return mAudioEngine ? mAudioEngine->outputMode() : QAudioEngine::Stereo;
}

void TSpatialAudio::setListenerPosition(float x, float y, float z)
{
    if (mAudioListener) {
        mAudioListener->setPosition(QVector3D(x, y, z));
    }
}

void TSpatialAudio::setListenerPosition(const QVector3D& position)
{
    if (mAudioListener) {
        mAudioListener->setPosition(position);
    }
}

QVector3D TSpatialAudio::listenerPosition() const
{
    return mAudioListener ? mAudioListener->position() : QVector3D();
}

void TSpatialAudio::setListenerRotation(float yaw, float pitch, float roll)
{
    if (mAudioListener) {
        QQuaternion rotation = QQuaternion::fromEulerAngles(pitch, yaw, roll);
        mAudioListener->setRotation(rotation);
    }
}

void TSpatialAudio::setListenerRotation(const QQuaternion& rotation)
{
    if (mAudioListener) {
        mAudioListener->setRotation(rotation);
    }
}

QQuaternion TSpatialAudio::listenerRotation() const
{
    return mAudioListener ? mAudioListener->rotation() : QQuaternion();
}

void TSpatialAudio::setMasterVolume(float volume)
{
    if (mAudioEngine) {
        mAudioEngine->setMasterVolume(qBound(0.0f, volume, 1.0f));
    }
}

float TSpatialAudio::masterVolume() const
{
    return mAudioEngine ? mAudioEngine->masterVolume() : 0.0f;
}

TSpatialAudioSource* TSpatialAudio::createSource(const QString& name, SourceProtocol protocol)
{
    if (!mAudioEngine) {
        qWarning() << "TSpatialAudio::createSource - Engine not initialized";
        return nullptr;
    }
    
    QMap<QString, std::shared_ptr<TSpatialAudioSource>>& sources = 
        (protocol == ProtocolAPI) ? mAPISpatialSources : mGMCPSpatialSources;
    
    if (sources.contains(name)) {
        qWarning() << "TSpatialAudio::createSource - Source already exists:" << name;
        return sources[name].get();
    }
    
    auto source = std::make_shared<TSpatialAudioSource>(mAudioEngine, name);
    sources[name] = source;
    
    return source.get();
}

TSpatialAudioSource* TSpatialAudio::getSource(const QString& name, SourceProtocol protocol)
{
    const QMap<QString, std::shared_ptr<TSpatialAudioSource>>& sources = 
        (protocol == ProtocolAPI) ? mAPISpatialSources : mGMCPSpatialSources;
    
    auto it = sources.find(name);
    return it != sources.end() ? it.value().get() : nullptr;
}

bool TSpatialAudio::removeSource(const QString& name, SourceProtocol protocol)
{
    QMap<QString, std::shared_ptr<TSpatialAudioSource>>& sources = 
        (protocol == ProtocolAPI) ? mAPISpatialSources : mGMCPSpatialSources;
    
    auto it = sources.find(name);
    if (it != sources.end()) {
        it.value()->stop();
        sources.erase(it);
        return true;
    }
    return false;
}

void TSpatialAudio::stopAllSources(SourceProtocol protocol)
{
    QMap<QString, std::shared_ptr<TSpatialAudioSource>>& sources = 
        (protocol == ProtocolAPI) ? mAPISpatialSources : mGMCPSpatialSources;
    
    for (auto& source : sources) {
        source->stop();
    }
}

void TSpatialAudio::removeAllSources(SourceProtocol protocol)
{
    stopAllSources(protocol);
    
    if (protocol == ProtocolAPI) {
        mAPISpatialSources.clear();
    } else {
        mGMCPSpatialSources.clear();
    }
}

QStringList TSpatialAudio::listSources(SourceProtocol protocol) const
{
    const QMap<QString, std::shared_ptr<TSpatialAudioSource>>& sources = 
        (protocol == ProtocolAPI) ? mAPISpatialSources : mGMCPSpatialSources;
    
    return sources.keys();
}

TSpatialAudioRoom* TSpatialAudio::createRoom()
{
    if (!mAudioEngine) {
        qWarning() << "TSpatialAudio::createRoom - Engine not initialized";
        return nullptr;
    }
    
    if (mRoom) {
        qWarning() << "TSpatialAudio::createRoom - Room already exists";
        return mRoom;
    }
    
    mRoom = new TSpatialAudioRoom(mAudioEngine);
    return mRoom;
}

TSpatialAudioRoom* TSpatialAudio::getRoom()
{
    return mRoom;
}

void TSpatialAudio::removeRoom()
{
    if (mRoom) {
        delete mRoom;
        mRoom = nullptr;
    }
}

// File resolution and caching (similar to TMedia)
QString TSpatialAudio::resolveFilePath(const QString& key, const QString& fileName, const QString& url, SourceProtocol protocol)
{
    // If fileName is provided and is an absolute path, copy it to media folder
    if (!fileName.isEmpty()) {
        QString normalizedPath = fileName;
        normalizedPath.replace('\\', '/');
        
        // Check if the file format is supported
        QFileInfo fileInfo(normalizedPath);
        const QString fileExtension = fileInfo.suffix();
        if (!isFormatSupported(fileExtension)) {
            qWarning() << "TSpatialAudio::resolveFilePath - Unsupported audio format:" << fileExtension 
                       << "for file:" << fileName << "Supported formats:" << getSupportedAudioFormats();
            return QString();
        }
        if (fileInfo.isAbsolute()) {
            // Copy absolute path file to profile media folder
            const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
            QDir().mkpath(mediaPath);
            
            const QString targetFileName = fileInfo.fileName();
            const QString targetPath = qsl("%1/%2").arg(mediaPath, targetFileName);
            
            // Copy file if it doesn't exist or is different
            if (!QFile::exists(targetPath) || QFileInfo(normalizedPath).size() != QFileInfo(targetPath).size()) {
                QFile::remove(targetPath);  // Remove old version if exists
                if (QFile::copy(normalizedPath, targetPath)) {
                    return targetPath;
                } else {
                    qWarning() << "TSpatialAudio::resolveFilePath - Failed to copy file" << normalizedPath << "to" << targetPath;
                    return QString();
                }
            }
            return targetPath;
        } else {
            // Relative path - check in media folder
            const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
            const QString fullPath = qsl("%1/%2").arg(mediaPath, normalizedPath);
            
            if (QFile::exists(fullPath)) {
                return fullPath;
            }
            
            // File doesn't exist - if URL provided, download it (url + fileName like TMedia)
            if (!url.isEmpty()) {
                downloadFile(key, normalizedPath, url);
                return QString();  // Will be available after download
            }
            
            qWarning() << "TSpatialAudio::resolveFilePath - File not found:" << fullPath;
            return QString();
        }
    } else if (!url.isEmpty()) {
        // Only URL provided without fileName - this is an error
        qWarning() << "TSpatialAudio::resolveFilePath - URL provided but no fileName specified";
        return QString();
    }
    
    return QString();
}

bool TSpatialAudio::isFileInCache(const QString& fileName) const
{
    const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    const QString fullPath = qsl("%1/%2").arg(mediaPath, fileName);
    return QFile::exists(fullPath);
}

void TSpatialAudio::downloadFile(const QString& key, const QString& fileName, const QString& url, SourceProtocol protocol)
{
    const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    const QDir mediaDir(mediaPath);
    
    if (!mediaDir.mkpath(mediaPath)) {
        qWarning() << "TSpatialAudio::downloadFile - Failed to create media directory:" << mediaPath;
        return;
    }
    
    // Create subdirectories if fileName contains slashes
    if (fileName.contains('/')) {
        const QString subPath = qsl("%1/%2").arg(mediaPath, fileName.section('/', 0, -2));
        if (!QDir().mkpath(subPath)) {
            qWarning() << "TSpatialAudio::downloadFile - Failed to create subdirectory:" << subPath;
            return;
        }
    }
    
    // Construct the full URL: url + fileName (like TMedia does)
    QString fullUrl = url;
    if (!fullUrl.endsWith('/')) {
        fullUrl += '/';
    }
    fullUrl += fileName;
    
    QUrl fileUrl = QUrl::fromUserInput(fullUrl);
    
    if (!fileUrl.isValid()) {
        qWarning() << "TSpatialAudio::downloadFile - Invalid URL:" << fullUrl;
        return;
    }
    
    QNetworkRequest request(fileUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, qsl("Mudlet/%1").arg(APP_VERSION));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    
#if !defined(QT_NO_SSL)
    if (fileUrl.scheme() == qsl("https")) {
        const QSslConfiguration config(QSslConfiguration::defaultConfiguration());
        request.setSslConfiguration(config);
    }
#endif
    
    mpHost->updateProxySettings(mNetworkAccessManager);
    QNetworkReply* reply = mNetworkAccessManager->get(request);
    mPendingDownloads[reply] = PendingDownload{key, fileName, protocol};
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::downloadFile - Starting download:" << fullUrl 
             << "for key:" << key << "fileName:" << fileName;
#endif
}

void TSpatialAudio::downloadFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    
    if (!mPendingDownloads.contains(reply)) {
        return;
    }
    
    const PendingDownload download = mPendingDownloads.take(reply);
    const QString& key = download.key;
    const QString& fileName = download.fileName;
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "TSpatialAudio::downloadFinished - Download failed for key:" << key 
                   << "fileName:" << fileName << "Error:" << reply->errorString();
        return;
    }
    
    const QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qWarning() << "TSpatialAudio::downloadFinished - Downloaded file is empty for key:" << key;
        return;
    }
    
    // Use the fileName we stored (not from URL)
    const QString mediaPath = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    const QString filePath = qsl("%1/%2").arg(mediaPath, fileName);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "TSpatialAudio::downloadFinished - Failed to open file for writing:" << filePath;
        return;
    }
    
    file.write(data);
    file.close();
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::downloadFinished - Successfully downloaded and saved:" << filePath;
#endif
    
    // If source already exists, set the file (check both API and GMCP based on protocol)
    TSpatialAudioSource* source = getSource(download.key, download.protocol);
    if (source) {
        source->setSource(filePath);
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::downloadFinished - Automatically loaded sound into existing source:" << download.key;
#endif
    }
}

// ====================================================================================
// GMCP Message Parsing (like TMedia::parseGMCP)
// ====================================================================================

void TSpatialAudio::parseGMCP(QString& packageMessage, QString& gmcp)
{
    if (!mpHost->mAcceptServerMedia) {
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::parseGMCP - Server media disabled, ignoring:" << packageMessage;
#endif
        return;
    }

    auto document = QJsonDocument::fromJson(gmcp.toUtf8());
    if (!document.isObject()) {
        qWarning() << "TSpatialAudio::parseGMCP - Invalid JSON in GMCP message:" << packageMessage;
        return;
    }

    auto json = document.object();
    const QString package = packageMessage.toLower();

#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::parseGMCP - Processing:" << package;
#endif

    if (package == "client.media.spatial.play") {
        parseJSONForSpatialPlay(json, ProtocolGMCP);
    } else if (package == "client.media.spatial.stop") {
        parseJSONForSpatialStop(json, ProtocolGMCP);
    } else if (package == "client.media.spatial.update") {
        parseJSONForSpatialUpdate(json, ProtocolGMCP);
    } else if (package == "client.media.spatial.listener") {
        parseJSONForSpatialListener(json);
    } else if (package == "client.media.spatial.capabilities") {
        sendCapabilitiesResponse();
    } else if (package == "client.media.spatial.settings") {
        sendSettingsResponse();
    } else if (package == "client.media.spatial.status") {
        sendStatusResponse();
    }
}

// Parse JSON fields
QString TSpatialAudio::parseJSONByKey(QJsonObject& json)
{
    return json[qsl("key")].toString();
}

QString TSpatialAudio::parseJSONByName(QJsonObject& json)
{
    return json[qsl("name")].toString();
}

QString TSpatialAudio::parseJSONByUrl(QJsonObject& json)
{
    return json[qsl("url")].toString();
}

float TSpatialAudio::parseJSONByVolume(QJsonObject& json)
{
    return json.contains(qsl("volume")) ? json[qsl("volume")].toDouble(80.0) / 100.0f : 0.8f;
}

int TSpatialAudio::parseJSONByLoops(QJsonObject& json)
{
    return json.contains(qsl("loops")) ? json[qsl("loops")].toInt(1) : 1;
}

float TSpatialAudio::parseJSONByOcclusion(QJsonObject& json)
{
    return json.contains(qsl("occlusion")) ? static_cast<float>(json[qsl("occlusion")].toDouble(0.0)) : 0.0f;
}

bool TSpatialAudio::parseJSONByPosition(QJsonObject& json, float& azimuth, float& elevation, float& distance)
{
    if (!json.contains(qsl("position"))) {
        return false;
    }

    const QJsonValue posValue = json[qsl("position")];
    
    // Support both array [azimuth, elevation, distance] and object {azimuth, elevation, distance}
    if (posValue.isArray()) {
        const QJsonArray posArray = posValue.toArray();
        if (posArray.size() >= 3) {
            azimuth = static_cast<float>(posArray[0].toDouble(0.0));
            elevation = static_cast<float>(posArray[1].toDouble(0.0));
            distance = static_cast<float>(posArray[2].toDouble(1.0));
            return true;
        }
    } else if (posValue.isObject()) {
        const QJsonObject posObj = posValue.toObject();
        azimuth = static_cast<float>(posObj[qsl("azimuth")].toDouble(0.0));
        elevation = static_cast<float>(posObj[qsl("elevation")].toDouble(0.0));
        distance = static_cast<float>(posObj[qsl("distance")].toDouble(1.0));
        return true;
    }
    
    return false;
}

bool TSpatialAudio::parseJSONByListenerPosition(QJsonObject& json, float& x, float& y, float& z)
{
    if (!json.contains(qsl("position"))) {
        return false;
    }

    const QJsonValue posValue = json[qsl("position")];
    
    if (posValue.isArray()) {
        const QJsonArray posArray = posValue.toArray();
        if (posArray.size() >= 3) {
            x = static_cast<float>(posArray[0].toDouble(0.0));
            y = static_cast<float>(posArray[1].toDouble(0.0));
            z = static_cast<float>(posArray[2].toDouble(0.0));
            return true;
        }
    } else if (posValue.isObject()) {
        const QJsonObject posObj = posValue.toObject();
        x = static_cast<float>(posObj[qsl("x")].toDouble(0.0));
        y = static_cast<float>(posObj[qsl("y")].toDouble(0.0));
        z = static_cast<float>(posObj[qsl("z")].toDouble(0.0));
        return true;
    }
    
    return false;
}

bool TSpatialAudio::parseJSONByListenerRotation(QJsonObject& json, float& yaw, float& pitch, float& roll)
{
    if (!json.contains(qsl("rotation"))) {
        return false;
    }

    const QJsonValue rotValue = json[qsl("rotation")];
    
    if (rotValue.isArray()) {
        const QJsonArray rotArray = rotValue.toArray();
        if (rotArray.size() >= 3) {
            yaw = static_cast<float>(rotArray[0].toDouble(0.0));
            pitch = static_cast<float>(rotArray[1].toDouble(0.0));
            roll = static_cast<float>(rotArray[2].toDouble(0.0));
            return true;
        }
    } else if (rotValue.isObject()) {
        const QJsonObject rotObj = rotValue.toObject();
        yaw = static_cast<float>(rotObj[qsl("yaw")].toDouble(0.0));
        pitch = static_cast<float>(rotObj[qsl("pitch")].toDouble(0.0));
        roll = static_cast<float>(rotObj[qsl("roll")].toDouble(0.0));
        return true;
    }
    
    return false;
}

bool TSpatialAudio::parseJSONByRoom(QJsonObject& json, QVector3D& dimensions, float& reverb, float& reflection, QString& material)
{
    if (!json.contains(qsl("room"))) {
        return false;
    }

    const QJsonObject roomObj = json[qsl("room")].toObject();
    
    // Parse dimensions
    if (roomObj.contains(qsl("dimensions"))) {
        const QJsonValue dimValue = roomObj[qsl("dimensions")];
        if (dimValue.isArray()) {
            const QJsonArray dimArray = dimValue.toArray();
            if (dimArray.size() >= 3) {
                dimensions = QVector3D(
                    static_cast<float>(dimArray[0].toDouble(10.0)),
                    static_cast<float>(dimArray[1].toDouble(10.0)),
                    static_cast<float>(dimArray[2].toDouble(3.0)));
            }
        }
    }
    
    reverb = static_cast<float>(roomObj[qsl("reverb")].toDouble(1.0));
    reflection = static_cast<float>(roomObj[qsl("reflection")].toDouble(1.0));
    material = roomObj[qsl("material")].toString(qsl("brick"));
    
    return true;
}

// GMCP action parsers
void TSpatialAudio::parseJSONForSpatialPlay(QJsonObject& json, SourceProtocol protocol)
{
    const QString key = parseJSONByKey(json);
    const QString name = parseJSONByName(json);
    
    if (key.isEmpty() || name.isEmpty()) {
        qWarning() << "TSpatialAudio::parseJSONForSpatialPlay - Missing key or name in GMCP message";
        return;
    }

#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::parseJSONForSpatialPlay - key:" << key << "name:" << name;
#endif

    // Get or create source
    TSpatialAudioSource* source = getSource(key, protocol);
    if (!source) {
        source = createSource(key, protocol);
    }
    
    if (!source) {
        qWarning() << "TSpatialAudio::parseJSONForSpatialPlay - Failed to create source:" << key;
        return;
    }

    // Parse URL and resolve file path
    const QString url = parseJSONByUrl(json);
    const QString filePath = resolveFilePath(key, name, url, protocol);
    
    if (!filePath.isEmpty()) {
        source->setSource(filePath);
    }

    // Set volume
    const float volume = parseJSONByVolume(json);
    source->setVolume(volume);

    // Set loops
    const int loops = parseJSONByLoops(json);
    source->setLoops(loops);

    // Set occlusion
    const float occlusion = parseJSONByOcclusion(json);
    if (occlusion > 0.0f) {
        source->setOcclusion(occlusion);
    }

    // Set position
    float azimuth, elevation, distance;
    if (parseJSONByPosition(json, azimuth, elevation, distance)) {
        source->setPosition(azimuth, elevation, distance);
    }

    // Handle room acoustics
    QVector3D dimensions;
    float reverb, reflection;
    QString material;
    if (parseJSONByRoom(json, dimensions, reverb, reflection, material)) {
        TSpatialAudioRoom* room = getRoom();
        if (!room) {
            room = createRoom();
        }
        if (room) {
            room->setDimensions(dimensions);
            room->setReverbGain(reverb);
            room->setReflectionGain(reflection);
            
            // Convert material string to enum and apply to all walls
            const QAudioRoom::Material materialEnum = stringToMaterial(material);
            room->setWallMaterial(QAudioRoom::Wall::LeftWall, materialEnum);
            room->setWallMaterial(QAudioRoom::Wall::RightWall, materialEnum);
            room->setWallMaterial(QAudioRoom::Wall::FrontWall, materialEnum);
            room->setWallMaterial(QAudioRoom::Wall::BackWall, materialEnum);
            room->setWallMaterial(QAudioRoom::Wall::Floor, materialEnum);
            room->setWallMaterial(QAudioRoom::Wall::Ceiling, materialEnum);
        }
    }

    // Check mute state before playing
    if (mudlet* pMudlet = mudlet::self()) {
        const bool isMuted = (protocol == ProtocolGMCP && pMudlet->muteGame()) 
                          || (protocol == ProtocolAPI && pMudlet->muteAPI());
        
        if (isMuted) {
#ifdef DEBUG_SPATIAL_AUDIO
            qDebug() << "TSpatialAudio::parseJSONForSpatialPlay - Not playing due to mute state:" << key;
#endif
            // Don't remove the source - leave it configured so updates work when unmuted
            return;
        }
    }

    // Play the sound
    source->play();
}

void TSpatialAudio::parseJSONForSpatialStop(QJsonObject& json, SourceProtocol protocol)
{
    const QString key = parseJSONByKey(json);
    
    if (key.isEmpty()) {
        // Stop all sources for this protocol
        stopAllSources(protocol);
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::parseJSONForSpatialStop - Stopped all sources";
#endif
        return;
    }

    TSpatialAudioSource* source = getSource(key, protocol);
    if (source) {
        source->stop();
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::parseJSONForSpatialStop - Stopped source:" << key;
#endif
    }
}

void TSpatialAudio::parseJSONForSpatialUpdate(QJsonObject& json, SourceProtocol protocol)
{
    const QString key = parseJSONByKey(json);
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::parseJSONForSpatialUpdate - Called with key:" << key;
#endif
    
    if (key.isEmpty()) {
        qWarning() << "TSpatialAudio::parseJSONForSpatialUpdate - Missing key in GMCP message";
        return;
    }

    TSpatialAudioSource* source = getSource(key, protocol);
    if (!source) {
        qWarning() << "TSpatialAudio::parseJSONForSpatialUpdate - Source not found:" << key << "protocol:" << (protocol == ProtocolAPI ? "API" : "GMCP");
        return;
    }
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::parseJSONForSpatialUpdate - Source found:" << key;
#endif

    // Update position
    float azimuth, elevation, distance;
    if (parseJSONByPosition(json, azimuth, elevation, distance)) {
        source->setPosition(azimuth, elevation, distance);
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::parseJSONForSpatialUpdate - Updated position for:" << key;
#endif
    }

    // Update volume
    if (json.contains(qsl("volume"))) {
        const float volume = parseJSONByVolume(json);
        source->setVolume(volume);
    }

    // Update occlusion
    if (json.contains(qsl("occlusion"))) {
        const float occlusion = parseJSONByOcclusion(json);
        source->setOcclusion(occlusion);
    }

    // If source has loops configured (meaning it should be playing) but isn't currently playing,
    // check if we're no longer muted and start it
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::parseJSONForSpatialUpdate - Source state:" << key 
             << "loops=" << source->loops() 
             << "isPlaying=" << source->isPlaying() 
             << "isPaused=" << source->isPaused();
#endif
    
    if (source->loops() != 0 && !source->isPlaying() && !source->isPaused()) {
        if (mudlet* pMudlet = mudlet::self()) {
            const bool isMuted = (protocol == ProtocolGMCP && pMudlet->muteGame()) 
                              || (protocol == ProtocolAPI && pMudlet->muteAPI());
            
#ifdef DEBUG_SPATIAL_AUDIO
            qDebug() << "TSpatialAudio::parseJSONForSpatialUpdate - Not playing, isMuted=" << isMuted;
#endif
            
            if (!isMuted) {
                qDebug() << "TSpatialAudio::parseJSONForSpatialUpdate - Starting playback for previously muted source:" << key;
                source->play();
            }
        }
    }
}

void TSpatialAudio::parseJSONForSpatialListener(QJsonObject& json)
{
    // Update listener position
    float x, y, z;
    if (parseJSONByListenerPosition(json, x, y, z)) {
        setListenerPosition(x, y, z);
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::parseJSONForSpatialListener - Updated position:" << x << y << z;
#endif
    }

    // Update listener rotation
    float yaw, pitch, roll;
    if (parseJSONByListenerRotation(json, yaw, pitch, roll)) {
        setListenerRotation(yaw, pitch, roll);
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::parseJSONForSpatialListener - Updated rotation:" << yaw << pitch << roll;
#endif
    }
}

// ============================================================================
// Test tone generation
// ============================================================================

QString TSpatialAudio::generateTestTone(ToneType type, float frequency, float durationSeconds, int sampleRate)
{
    if (durationSeconds <= 0.0f || sampleRate <= 0) {
        qWarning() << "TSpatialAudio::generateTestTone - Invalid parameters";
        return QString();
    }

    const int numSamples = static_cast<int>(durationSeconds * sampleRate);
    const int numChannels = 1;  // Mono
    const int bitsPerSample = 16;
    const int byteRate = sampleRate * numChannels * bitsPerSample / 8;
    const int blockAlign = numChannels * bitsPerSample / 8;
    
    // Generate audio samples
    QVector<qint16> samples(numSamples);
    
    switch (type) {
        case WhiteNoise: {
            // White noise: random samples with uniform distribution
            for (int i = 0; i < numSamples; ++i) {
                samples[i] = static_cast<qint16>((QRandomGenerator::global()->bounded(65536) - 32768) * 0.3);  // 30% amplitude
            }
            break;
        }
        
        case PinkNoise: {
            // Pink noise using Voss-McCartney algorithm (simplified)
            // Pink noise has equal energy per octave
            float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
            
            for (int i = 0; i < numSamples; ++i) {
                const float white = (QRandomGenerator::global()->bounded(65536) / 32768.0f) - 1.0f;
                b0 = 0.99886f * b0 + white * 0.0555179f;
                b1 = 0.99332f * b1 + white * 0.0750759f;
                b2 = 0.96900f * b2 + white * 0.1538520f;
                b3 = 0.86650f * b3 + white * 0.3104856f;
                b4 = 0.55000f * b4 + white * 0.5329522f;
                b5 = -0.7616f * b5 - white * 0.0168980f;
                const float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
                b6 = white * 0.115926f;
                
                samples[i] = static_cast<qint16>(pink * 4096.0f);  // Scale appropriately
            }
            break;
        }
        
        case SineWave: {
            // Pure sine wave at specified frequency
            const float angularFreq = 2.0f * M_PI * frequency / sampleRate;
            for (int i = 0; i < numSamples; ++i) {
                const float sample = std::sin(angularFreq * i);
                samples[i] = static_cast<qint16>(sample * 16384.0f);  // 50% amplitude
            }
            break;
        }
    }
    
    // Create WAV file in profile's media directory
    const QString mediaDir = mudlet::getMudletPath(enums::profileMediaPath, mpHost->getName());
    
    // Ensure media directory exists
    QDir dir;
    if (!dir.mkpath(mediaDir)) {
        qWarning() << "TSpatialAudio::generateTestTone - Failed to create media directory:" << mediaDir;
        return QString();
    }
    
    QString fileName;
    switch (type) {
        case WhiteNoise:
            fileName = qsl("mudlet_white_noise_%1s_%2hz.wav").arg(durationSeconds).arg(sampleRate);
            break;
        case PinkNoise:
            fileName = qsl("mudlet_pink_noise_%1s_%2hz.wav").arg(durationSeconds).arg(sampleRate);
            break;
        case SineWave:
            fileName = qsl("mudlet_sine_%1hz_%2s_%3sr.wav").arg(frequency).arg(durationSeconds).arg(sampleRate);
            break;
    }
    
    const QString filePath = QDir(mediaDir).filePath(fileName);
    
    // Check if file already exists
    if (QFile::exists(filePath)) {
#ifdef DEBUG_SPATIAL_AUDIO
        qDebug() << "TSpatialAudio::generateTestTone - Using cached tone:" << filePath;
#endif
        return filePath;
    }
    
    // Write WAV file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "TSpatialAudio::generateTestTone - Failed to create file:" << filePath;
        return QString();
    }
    
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    
    // WAV header
    const quint32 dataSize = numSamples * numChannels * bitsPerSample / 8;
    const quint32 fileSize = dataSize + 36;
    
    // RIFF chunk
    file.write("RIFF");
    out << fileSize;
    file.write("WAVE");
    
    // fmt chunk
    file.write("fmt ");
    out << quint32(16);              // fmt chunk size
    out << quint16(1);               // PCM format
    out << quint16(numChannels);     // number of channels
    out << quint32(sampleRate);      // sample rate
    out << quint32(byteRate);        // byte rate
    out << quint16(blockAlign);      // block align
    out << quint16(bitsPerSample);   // bits per sample
    
    // data chunk
    file.write("data");
    out << dataSize;
    
    // Write samples
    for (qint16 sample : samples) {
        out << sample;
    }
    
    file.close();
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::generateTestTone - Generated tone:" << filePath;
#endif
    return filePath;
}

bool TSpatialAudio::createTestToneSource(const QString& name, ToneType type, float frequency, float durationSeconds)
{
    if (!isInitialized()) {
        qWarning() << "TSpatialAudio::createTestToneSource - Engine not initialized";
        return false;
    }
    
    // Generate the tone file
    const QString filePath = generateTestTone(type, frequency, durationSeconds);
    if (filePath.isEmpty()) {
        return false;
    }
    
    // Create or get the source
    TSpatialAudioSource* source = getSource(name, ProtocolAPI);
    if (!source) {
        source = createSource(name, ProtocolAPI);
        if (!source) {
            qWarning() << "TSpatialAudio::createTestToneSource - Failed to create source:" << name;
            return false;
        }
    }
    
    // Set the generated file as the source
    source->setSource(filePath);
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::createTestToneSource - Created test tone source:" << name;
    qDebug() << "TSpatialAudio::createTestToneSource - File path:" << filePath;
#endif
    
    // Verify the file exists and is readable
    QFile testFile(filePath);
    if (!testFile.exists()) {
        qWarning() << "TSpatialAudio::createTestToneSource - Generated file does not exist!";
        return false;
    }
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::createTestToneSource - File size:" << testFile.size() << "bytes";
#endif
    
    return true;
}

// GMCP Response Methods
// ====================================================================================

QStringList TSpatialAudio::getSupportedAudioFormats()
{
    QStringList supportedFormats;
    
    // Runtime detection of supported audio formats from Qt6 Multimedia
    // This replaces hardcoded format lists and adapts to the actual capabilities
    // of the multimedia backend (FFmpeg, GStreamer, AVFoundation, etc.)
    QMediaFormat format;
    const QList<QMediaFormat::FileFormat> fileFormats = format.supportedFileFormats(QMediaFormat::Decode);
    
    // Map Qt's enum values to string extensions
    for (const auto& format : fileFormats) {
        switch (format) {
            case QMediaFormat::Wave:
                if (!supportedFormats.contains(qsl("wav"))) {
                    supportedFormats << qsl("wav");
                }
                break;
            case QMediaFormat::MP3:
                if (!supportedFormats.contains(qsl("mp3"))) {
                    supportedFormats << qsl("mp3");
                }
                break;
            case QMediaFormat::Ogg:
                if (!supportedFormats.contains(qsl("ogg"))) {
                    supportedFormats << qsl("ogg");
                }
                break;
            case QMediaFormat::FLAC:
                if (!supportedFormats.contains(qsl("flac"))) {
                    supportedFormats << qsl("flac");
                }
                break;
            case QMediaFormat::AAC:
                if (!supportedFormats.contains(qsl("aac"))) {
                    supportedFormats << qsl("aac");
                }
                break;
            case QMediaFormat::Mpeg4Audio:
                if (!supportedFormats.contains(qsl("m4a"))) {
                    supportedFormats << qsl("m4a");
                }
                break;
            default:
                // Skip other formats (video formats, etc.)
                break;
        }
    }
    
    // Ensure we always support WAV as a fallback (since we can generate test tones)
    if (!supportedFormats.contains(qsl("wav"))) {
        supportedFormats.prepend(qsl("wav"));
    }
    
    // Sort for consistent ordering
    supportedFormats.sort();
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::getSupportedAudioFormats - Detected formats:" << supportedFormats;
#endif
    
    return supportedFormats;
}

bool TSpatialAudio::isFormatSupported(const QString& fileExtension)
{
    // Check if format is in our supported list
    const QStringList supportedFormats = getSupportedAudioFormats();
    
    // Normalize extension (remove leading dot if present, convert to lowercase)
    QString normalizedExt = fileExtension.toLower();
    if (normalizedExt.startsWith('.')) {
        normalizedExt = normalizedExt.mid(1);
    }
    
    // Check against supported formats
    bool isSupported = supportedFormats.contains(normalizedExt);
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::isFormatSupported - Extension:" << normalizedExt 
             << "Supported:" << isSupported << "Available formats:" << supportedFormats;
#endif
    
    return isSupported;
}

QStringList TSpatialAudio::getSupportedRoomMaterials()
{
    // Return all Qt6 QAudioRoom::Material enum values as strings
    // This ensures bidirectional compatibility between string names and Qt enums
    QStringList materials;
    
    materials << qsl("brick");          // QAudioRoom::Material::BrickBare
    materials << qsl("brickpainted");   // QAudioRoom::Material::BrickPainted  
    materials << qsl("concrete");       // QAudioRoom::Material::ConcreteBlockCoarse
    materials << qsl("concretepainted"); // QAudioRoom::Material::ConcreteBlockPainted
    materials << qsl("curtainheavy");   // QAudioRoom::Material::CurtainHeavy
    materials << qsl("fiberglassinsulation"); // QAudioRoom::Material::FiberGlassInsulation
    materials << qsl("glassthin");      // QAudioRoom::Material::GlassThin
    materials << qsl("glassthick");     // QAudioRoom::Material::GlassThick
    materials << qsl("grass");          // QAudioRoom::Material::Grass

    materials << qsl("marble");         // QAudioRoom::Material::Marble
    materials << qsl("metal");          // QAudioRoom::Material::Metal

    materials << qsl("plasterrough");   // QAudioRoom::Material::PlasterRough
    materials << qsl("plastersmooth");  // QAudioRoom::Material::PlasterSmooth
    materials << qsl("plywoodpanel");   // QAudioRoom::Material::PlywoodPanel
    materials << qsl("polishedconcreteortile"); // QAudioRoom::Material::PolishedConcreteOrTile
    materials << qsl("sheetrock");      // QAudioRoom::Material::Sheetrock

    materials << qsl("woodceiling");    // QAudioRoom::Material::WoodCeiling
    materials << qsl("woodpanel");      // QAudioRoom::Material::WoodPanel
    materials << qsl("transparent");    // QAudioRoom::Material::Transparent
    materials << qsl("acousticceilingtiles"); // QAudioRoom::Material::AcousticCeilingTiles
    materials << qsl("linoleumonconcrete"); // QAudioRoom::Material::LinoleumOnConcrete
    materials << qsl("parquetonconcrete"); // QAudioRoom::Material::ParquetOnConcrete
    materials << qsl("wateroricesurface"); // QAudioRoom::Material::WaterOrIceSurface
    materials << qsl("uniform");        // QAudioRoom::Material::UniformMaterial
    
    return materials;
}

QAudioRoom::Material TSpatialAudio::stringToMaterial(const QString& materialName)
{
    const QString name = materialName.toLower().replace("_", "").replace("-", "");
    
    // Map string names to Qt enum values with comprehensive coverage
    // Note: Use "transparent" or "air" for open areas with no walls/reflections
    if (name == "brick" || name == "brickbare") {
        return QAudioRoom::Material::BrickBare;
    } else if (name == "brickpainted") {
        return QAudioRoom::Material::BrickPainted;
    } else if (name == "concrete" || name == "concreteblockcoarse") {
        return QAudioRoom::Material::ConcreteBlockCoarse;
    } else if (name == "concretepainted" || name == "concreteblockpainted") {
        return QAudioRoom::Material::ConcreteBlockPainted;
    } else if (name == "curtain" || name == "curtainheavy" || name == "fabric") {
        return QAudioRoom::Material::CurtainHeavy;
    } else if (name == "fiberglass" || name == "fiberglassinsulation" || name == "carpet") {
        return QAudioRoom::Material::FiberGlassInsulation;
    } else if (name == "glass" || name == "glassthin") {
        return QAudioRoom::Material::GlassThin;
    } else if (name == "glassthick") {
        return QAudioRoom::Material::GlassThick;
    } else if (name == "grass") {
        return QAudioRoom::Material::Grass;
    } else if (name == "linoleum" || name == "linoleumontile") {
        return QAudioRoom::Material::PolishedConcreteOrTile; // Use as hard floor surface
    } else if (name == "marble") {
        return QAudioRoom::Material::Marble;
    } else if (name == "metal") {
        return QAudioRoom::Material::Metal;
    } else if (name == "parquet" || name == "parquetonfiberboard" || name == "parquetonconcrete") {
        return QAudioRoom::Material::ParquetOnConcrete;
    } else if (name == "plaster" || name == "plasterrough") {
        return QAudioRoom::Material::PlasterRough;
    } else if (name == "plastersmooth") {
        return QAudioRoom::Material::PlasterSmooth;
    } else if (name == "plywood" || name == "plywoodpanel") {
        return QAudioRoom::Material::PlywoodPanel;
    } else if (name == "tile" || name == "polishedconcrete" || name == "polishedconcreteortile") {
        return QAudioRoom::Material::PolishedConcreteOrTile;
    } else if (name == "sheetrock" || name == "drywall") {
        return QAudioRoom::Material::Sheetrock;
    } else if (name == "water" || name == "ice" || name == "wateroriceereflector" || name == "wateroricesurface") {
        return QAudioRoom::Material::WaterOrIceSurface;
    } else if (name == "woodceiling") {
        return QAudioRoom::Material::WoodCeiling;
    } else if (name == "wood" || name == "woodpanel") {
        return QAudioRoom::Material::WoodPanel;
    } else if (name == "transparent" || name == "air") {
        return QAudioRoom::Material::Transparent;
    } else if (name == "acousticceilingtiles" || name == "acoustictiles") {
        return QAudioRoom::Material::AcousticCeilingTiles;
    } else if (name == "linoleumonconcrete" || name == "linoleum") {
        return QAudioRoom::Material::LinoleumOnConcrete;
    } else if (name == "uniform" || name == "uniformmaterial") {
        return QAudioRoom::Material::UniformMaterial;
    }
    
    // Default fallback
    return QAudioRoom::Material::BrickBare;
}

QString TSpatialAudio::materialToString(QAudioRoom::Material material)
{
    // Convert Qt enum back to string names for GMCP responses
    switch (material) {
        case QAudioRoom::Material::BrickBare:
            return qsl("brick");
        case QAudioRoom::Material::BrickPainted:
            return qsl("brickpainted");
        case QAudioRoom::Material::ConcreteBlockCoarse:
            return qsl("concrete");
        case QAudioRoom::Material::ConcreteBlockPainted:
            return qsl("concretepainted");
        case QAudioRoom::Material::CurtainHeavy:
            return qsl("curtainheavy");
        case QAudioRoom::Material::FiberGlassInsulation:
            return qsl("fiberglassinsulation");
        case QAudioRoom::Material::GlassThin:
            return qsl("glassthin");
        case QAudioRoom::Material::GlassThick:
            return qsl("glassthick");
        case QAudioRoom::Material::Grass:
            return qsl("grass");

        case QAudioRoom::Material::Marble:
            return qsl("marble");
        case QAudioRoom::Material::Metal:
            return qsl("metal");

        case QAudioRoom::Material::PlasterRough:
            return qsl("plasterrough");
        case QAudioRoom::Material::PlasterSmooth:
            return qsl("plastersmooth");
        case QAudioRoom::Material::PlywoodPanel:
            return qsl("plywoodpanel");
        case QAudioRoom::Material::PolishedConcreteOrTile:
            return qsl("polishedconcreteortile");
        case QAudioRoom::Material::Sheetrock:
            return qsl("sheetrock");

        case QAudioRoom::Material::WoodCeiling:
            return qsl("woodceiling");
        case QAudioRoom::Material::WoodPanel:
            return qsl("woodpanel");
        case QAudioRoom::Material::Transparent:
            return qsl("transparent");
        case QAudioRoom::Material::AcousticCeilingTiles:
            return qsl("acousticceilingtiles");
        case QAudioRoom::Material::LinoleumOnConcrete:
            return qsl("linoleumonconcrete");
        case QAudioRoom::Material::ParquetOnConcrete:
            return qsl("parquetonconcrete");
        case QAudioRoom::Material::WaterOrIceSurface:
            return qsl("wateroricesurface");
        case QAudioRoom::Material::UniformMaterial:
            return qsl("uniform");
    }
    
    return qsl("brick");  // Default fallback
}

void TSpatialAudio::sendGMCPResponse(const QString& messageType, const QString& data)
{
    // Check connection status first
    if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState) {
        qWarning() << "TSpatialAudio::sendGMCPResponse - Not connected to game server";
        return;
    }

    if (!mpHost->mTelnet.isGMCPEnabled()) {
        qWarning() << "TSpatialAudio::sendGMCPResponse - GMCP is not currently enabled";
        return;
    }

    // Format the GMCP message using the same pattern as TLuaInterpreter::sendGMCP
    const std::string msg = mpHost->mTelnet.encodeAndCookBytes(messageType.toStdString());
    const std::string payload = mpHost->mTelnet.encodeAndCookBytes(data.toStdString());

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += msg;
    if (!payload.empty()) {
        output += " ";
        output += payload;
    }
    output += TN_IAC;
    output += TN_SE;

    // Send the raw GMCP message
    if (!mpHost->mTelnet.socketOutRaw(output)) {
        qWarning() << "TSpatialAudio::sendGMCPResponse - Failed to send GMCP message:" << messageType;
    }

#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::sendGMCPResponse - Sent:" << messageType << "data:" << data;
#endif
}

void TSpatialAudio::sendCapabilitiesResponse()
{
    QJsonObject capabilities;
    capabilities[qsl("version")] = qsl("1.0");
    
    // Supported audio formats - query at runtime
    QJsonArray formats;
    const QStringList supportedFormats = getSupportedAudioFormats();

    for (const QString& format : supportedFormats) {
        formats << format;
    }

    capabilities[qsl("formats")] = formats;
    
    // Supported output modes
    QJsonArray outputModes;
    outputModes << qsl("stereo") << qsl("surround") << qsl("headphone");
    capabilities[qsl("output_modes")] = outputModes;
    
    // Supported room materials - get complete list from Qt enum
    QJsonArray materials;
    const QStringList supportedMaterials = getSupportedRoomMaterials();

    for (const QString& material : supportedMaterials) {
        materials << material;
    }

    capabilities[qsl("room_materials")] = materials;
    
    // Technical capabilities
    capabilities[qsl("max_sources")] = 32;
    capabilities[qsl("distance_model")] = qsl("inverse");
    capabilities[qsl("coordinate_system")] = qsl("spherical");
    
    // Feature support
    QJsonObject features;
    features[qsl("positioning")] = true;
    features[qsl("room_acoustics")] = true;
    features[qsl("occlusion")] = true;
    features[qsl("listener_control")] = true;
    features[qsl("test_tones")] = true;
    features[qsl("volume_control")] = true;
    features[qsl("loops")] = true;
    capabilities[qsl("features")] = features;
    
    // Send response
    QJsonDocument doc(capabilities);
    QString response = doc.toJson(QJsonDocument::Compact);
    sendGMCPResponse(qsl("Client.Media.Spatial.Capabilities"), response);
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::sendCapabilitiesResponse - Sent capabilities:" << response;
#endif
}

void TSpatialAudio::sendSettingsResponse()
{
    QJsonObject settings;
    
    // Master volume (convert from 0.0-1.0 to 0-100)
    settings[qsl("master_volume")] = static_cast<int>(masterVolume() * 100);
    
    // Listener position and rotation
    QJsonObject listener;
    QVector3D pos = listenerPosition();
    QJsonArray position;
    position << pos.x() << pos.y() << pos.z();
    listener[qsl("position")] = position;
    
    QQuaternion rot = listenerRotation();
    QVector3D euler = rot.toEulerAngles();
    QJsonArray rotation;
    rotation << euler.x() << euler.y() << euler.z();
    listener[qsl("rotation")] = rotation;
    settings[qsl("listener")] = listener;
    
    // Room settings if available
    if (TSpatialAudioRoom* room = getRoom()) {
        QJsonObject roomObj;
        QVector3D dims = room->dimensions();
        QJsonArray dimensions;
        dimensions << dims.x() << dims.y() << dims.z();
        roomObj[qsl("dimensions")] = dimensions;
        
        roomObj[qsl("reverb_gain")] = room->reverbGain();
        roomObj[qsl("reflection_gain")] = room->reflectionGain();
        roomObj[qsl("reverb_time")] = room->reverbTime();
        roomObj[qsl("reverb_brightness")] = room->reverbBrightness();
        
        settings[qsl("room")] = roomObj;
    }
    
    // Send response
    QJsonDocument doc(settings);
    QString response = doc.toJson(QJsonDocument::Compact);
    sendGMCPResponse(qsl("Client.Media.Spatial.Settings"), response);
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::sendSettingsResponse - Sent settings:" << response;
#endif
}

void TSpatialAudio::sendStatusResponse()
{
    QJsonObject status;
    
    // Collect active GMCP sources only (server should not know about client API sources)
    QJsonArray activeSources;
    QStringList gmcpSources = listSources(ProtocolGMCP);
    
    int activeCount = 0;

    for (const QString& sourceKey : gmcpSources) {
        TSpatialAudioSource* source = getSource(sourceKey, ProtocolGMCP);
        
        if (source && source->isPlaying()) {
            QJsonObject sourceObj;
            sourceObj[qsl("key")] = sourceKey;
            sourceObj[qsl("status")] = source->isPaused() ? qsl("paused") : qsl("playing");
            
            // Position in spherical coordinates
            QJsonArray position;
            position << source->azimuth() << source->elevation() << source->distance();
            sourceObj[qsl("position")] = position;
            
            sourceObj[qsl("volume")] = static_cast<int>(source->volume() * 100);
            sourceObj[qsl("occlusion")] = source->occlusion();
            sourceObj[qsl("size")] = source->size();
            sourceObj[qsl("loops")] = source->loops();
            
            activeSources << sourceObj;
            activeCount++;
        }
    }
    
    status[qsl("active_sources")] = activeSources;
    status[qsl("source_count")] = activeCount;
    status[qsl("max_sources")] = 32;
    
    // Engine status
    status[qsl("engine_initialized")] = isInitialized();
    
    // Send response
    QJsonDocument doc(status);
    QString response = doc.toJson(QJsonDocument::Compact);
    sendGMCPResponse(qsl("Client.Media.Spatial.Status"), response);
    
#ifdef DEBUG_SPATIAL_AUDIO
    qDebug() << "TSpatialAudio::sendStatusResponse - Sent status:" << response;
#endif
}

