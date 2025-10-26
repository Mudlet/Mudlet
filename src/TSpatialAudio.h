#ifndef MUDLET_TSPATIALAUDIO_H
#define MUDLET_TSPATIALAUDIO_H

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

#include <QAudioEngine>
#include <QAudioListener>
#include <QAudioRoom>
#include <QSpatialSound>
#include <QString>
#include <QVector3D>
#include <QQuaternion>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

class Host;

// Spatial audio source representing a single positioned sound
class TSpatialAudioSource
{
public:
    TSpatialAudioSource(QAudioEngine* engine, const QString& name);
    ~TSpatialAudioSource();

    // Basic controls
    void play();
    void pause();
    void stop();
    bool isPlaying() const { return mIsPlaying; }
    bool isPaused() const { return mIsPaused; }
    
    // Source configuration
    void setSource(const QString& filePath);
    QString source() const { return mSource; }
    QString name() const { return mName; }
    
    // Position and orientation (spherical coordinates)
    void setPosition(float azimuth, float elevation, float distance);
    void setAzimuth(float degrees);    // -180 to 180, 0 = front
    void setElevation(float degrees);  // -90 to 90, 0 = level
    void setDistance(float meters);    // 0+
    
    float azimuth() const { return mAzimuth; }
    float elevation() const { return mElevation; }
    float distance() const { return mDistance; }
    
    // Volume and attenuation
    void setVolume(float volume);  // 0.0 to 1.0
    float volume() const;
    void setOcclusion(float amount);  // 0.0 to 4.0 - simulates obstacles
    float occlusion() const;
    
    // Size of the sound source (affects spread)
    void setSize(float size);  // meters
    float size() const;
    
    // Looping
    void setLoops(int count);  // -1 for infinite, 1 for once, 2+ for multiple
    int loops() const;
    
    // Direct access to underlying QSpatialSound
    QSpatialSound* spatialSound() { return mSpatialSound; }

private:
    void updateCartesianPosition();
    
    QString mName;
    QString mSource;
    QSpatialSound* mSpatialSound = nullptr;
    QAudioEngine* mAudioEngine = nullptr;  // Not owned
    
    // Spherical coordinates
    float mAzimuth = 0.0f;
    float mElevation = 0.0f;
    float mDistance = 1.0f;
    
    bool mIsPlaying = false;
    bool mIsPaused = false;
};

// Acoustic room/environment configuration
class TSpatialAudioRoom
{
public:
    explicit TSpatialAudioRoom(QAudioEngine* engine);
    ~TSpatialAudioRoom();
    
    // Room dimensions
    void setDimensions(float width, float height, float depth);
    void setDimensions(const QVector3D& dimensions);
    QVector3D dimensions() const;
    
    void setWallMaterial(QAudioRoom::Wall wall, QAudioRoom::Material material);
    
    // Reverb and reflection gains
    void setReflectionGain(float gain);  // 0.0 to 5.0
    float reflectionGain() const;
    void setReverbGain(float gain);      // 0.0 to 5.0
    float reverbGain() const;
    void setReverbTime(float seconds);   // 0.0 to 20.0
    float reverbTime() const;
    void setReverbBrightness(float brightness);  // -1.0 to 1.0
    float reverbBrightness() const;
    
    // Direct access to underlying QAudioRoom
    QAudioRoom* audioRoom() { return mAudioRoom; }

private:
    QAudioRoom* mAudioRoom = nullptr;
    QAudioEngine* mAudioEngine = nullptr;  // Not owned
};

// Main spatial audio manager
class TSpatialAudio : public QObject
{
    Q_OBJECT

public:
    enum SourceProtocol {
        ProtocolAPI = 0,   // From Lua API calls
        ProtocolGMCP = 1   // From GMCP server messages
    };

    explicit TSpatialAudio(Host* host);
    ~TSpatialAudio();
    
    // Engine management
    bool initialize();
    void shutdown();
    bool isInitialized() const { return mAudioEngine != nullptr; }
    
    // Output mode
    enum OutputMode {
        Stereo = QAudioEngine::Stereo,
        Surround = QAudioEngine::Surround,
        Headphone = QAudioEngine::Headphone
    };
    void setOutputMode(OutputMode mode);
    OutputMode outputMode() const;
    
    // Listener (player) position and orientation
    void setListenerPosition(float x, float y, float z);
    void setListenerPosition(const QVector3D& position);
    QVector3D listenerPosition() const;
    
    void setListenerRotation(float yaw, float pitch, float roll);
    void setListenerRotation(const QQuaternion& rotation);
    QQuaternion listenerRotation() const;
    
    // Master volume
    void setMasterVolume(float volume);  // 0.0 to 1.0
    float masterVolume() const;
    
    // Sound source management
    TSpatialAudioSource* createSource(const QString& name, SourceProtocol protocol = ProtocolAPI);
    TSpatialAudioSource* getSource(const QString& name, SourceProtocol protocol = ProtocolAPI);
    bool removeSource(const QString& name, SourceProtocol protocol = ProtocolAPI);
    void stopAllSources(SourceProtocol protocol = ProtocolAPI);
    void removeAllSources(SourceProtocol protocol = ProtocolAPI);
    QStringList listSources(SourceProtocol protocol = ProtocolAPI) const;
    
    // GMCP message parsing (like TMedia::parseGMCP)
    void parseGMCP(QString& packageMessage, QString& gmcp);
    
    // Room management
    TSpatialAudioRoom* createRoom();
    TSpatialAudioRoom* getRoom();
    void removeRoom();
    
    // Direct access to engine
    QAudioEngine* engine() { return mAudioEngine; }
    QAudioListener* listener() { return mAudioListener; }
    
    // File resolution and caching (similar to TMedia)
    QString resolveFilePath(const QString& key, const QString& fileName, const QString& url, SourceProtocol protocol = ProtocolAPI);
    bool isFileInCache(const QString& fileName) const;
    void downloadFile(const QString& key, const QString& fileName, const QString& url, SourceProtocol protocol = ProtocolAPI);
    
    // Test tone generation
    enum ToneType {
        WhiteNoise,
        PinkNoise,
        SineWave
    };
    QString generateTestTone(ToneType type, float frequency, float durationSeconds, int sampleRate = 48000);
    bool createTestToneSource(const QString& name, ToneType type, float frequency, float durationSeconds);

private slots:
    void downloadFinished(QNetworkReply* reply);

private:
    // GMCP JSON parsing methods (like TMedia)
    void parseJSONForSpatialPlay(QJsonObject& json, SourceProtocol protocol);
    void parseJSONForSpatialStop(QJsonObject& json, SourceProtocol protocol);
    void parseJSONForSpatialUpdate(QJsonObject& json, SourceProtocol protocol);
    void parseJSONForSpatialListener(QJsonObject& json);
    
    // JSON field parsers
    static QString parseJSONByKey(QJsonObject& json);
    static QString parseJSONByName(QJsonObject& json);
    static QString parseJSONByUrl(QJsonObject& json);
    static float parseJSONByVolume(QJsonObject& json);
    static int parseJSONByLoops(QJsonObject& json);
    static float parseJSONByOcclusion(QJsonObject& json);
    static bool parseJSONByPosition(QJsonObject& json, float& azimuth, float& elevation, float& distance);
    static bool parseJSONByListenerPosition(QJsonObject& json, float& x, float& y, float& z);
    static bool parseJSONByListenerRotation(QJsonObject& json, float& yaw, float& pitch, float& roll);
    static bool parseJSONByRoom(QJsonObject& json, QVector3D& dimensions, float& reverb, float& reflection, QString& material);

    Host* mpHost = nullptr;
    QAudioEngine* mAudioEngine = nullptr;
    QAudioListener* mAudioListener = nullptr;
    TSpatialAudioRoom* mRoom = nullptr;
    
    // Separate source maps for API and GMCP (like TMedia)
    QMap<QString, std::shared_ptr<TSpatialAudioSource>> mAPISpatialSources;
    QMap<QString, std::shared_ptr<TSpatialAudioSource>> mGMCPSpatialSources;
    
    QNetworkAccessManager* mNetworkAccessManager = nullptr;
    
    struct PendingDownload {
        QString key;
        QString fileName;
        SourceProtocol protocol;
    };
    QMap<QNetworkReply*, PendingDownload> mPendingDownloads;
};

#endif // MUDLET_TSPATIALAUDIO_H
