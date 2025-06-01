#ifndef MUDLET_LLAMAFILEMANAGER_H
#define MUDLET_LLAMAFILEMANAGER_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <memory>
#include <optional>
#include <functional>
#include "utils.h"

class LlamafileManager : public QObject {
    Q_OBJECT

public:
    enum class Status {
        Stopped,
        Starting,
        Running,
        Stopping,
        Error,
        Unhealthy
    };
    Q_ENUM(Status)

    enum class ApiType {
        ChatCompletions,
        TextCompletions,
        Embeddings,
        Models
    };
    Q_ENUM(ApiType)

    struct Config {
        QString modelPath;
        QString host = "127.0.0.1";
        int port = 8080;
        int startupTimeoutMs = 30000;
        int healthCheckIntervalMs = 30000;
        int maxRestartAttempts = 3;
        bool autoRestart = true;
        bool enableGpu = true;
        QStringList extraArgs;
    };

    struct ApiRequest {
        QString model = "LLaMA_CPP";
        QJsonObject messages; // For chat completions
        QString prompt;       // For text completions
        QStringList input;    // For embeddings
        double temperature = 0.7;
        int maxTokens = 150;
        bool stream = false;
        QJsonObject extraParams;
    };

    struct ApiResponse {
        bool success = false;
        QString error;
        QJsonObject data;
        int statusCode = 0;
    };

    using ApiCallback = std::function<void(const ApiResponse&)>;
    using StreamChunkCallback = std::function<void(const QString& chunk, bool isComplete)>;
    using StreamErrorCallback = std::function<void(const QString& error)>;

    explicit LlamafileManager(QObject* parent = nullptr);
    ~LlamafileManager();

    // Process management
    bool start(const Config& config);
    void stop();
    Status status() const noexcept { return currentStatus; }
    bool isRunning() const noexcept { return currentStatus == Status::Running; }
    std::optional<qint64> processId() const noexcept;
    
    // Configuration
    void setConfig(const Config& config) { this->config = config; }
    const Config& getConfig() const noexcept { return config; }
    
    // API calls
    void chatCompletion(const ApiRequest& request, ApiCallback callback);
    void textCompletion(const ApiRequest& request, ApiCallback callback);
    void embeddings(const ApiRequest& request, ApiCallback callback);
    void getModels(ApiCallback callback);
    void textCompletionStream(const ApiRequest& request, StreamChunkCallback chunkCallback, StreamErrorCallback errorCallback);
    
    // Health monitoring
    void enableHealthCheck(bool enable = true);
    bool isHealthy() const noexcept { return healthy; }
    
    // Utility functions
    static bool isLlamafileExecutable(const QString& path);
    static QString findLlamafileExecutable(const QStringList& searchPaths = {});
    QUrl apiBaseUrl() const;

signals:
    void statusChanged(Status newStatus, Status oldStatus);
    void processStarted();
    void processStopped();
    void processError(const QString& error);
    void healthCheckFailed(const QString& reason);
    void healthCheckPassed();
    void apiRequestCompleted(ApiType type, const ApiResponse& response);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessStateChanged(QProcess::ProcessState newState);
    void performHealthCheck();
    void onHealthCheckReply();

private:
    // Core members
    std::unique_ptr<QProcess> process;
    std::unique_ptr<QTimer> healthCheckTimer;
    std::unique_ptr<QNetworkAccessManager> networkManager;
    
    // State
    Config config;
    Status currentStatus = Status::Stopped;
    bool healthy = false;
    int restartAttempts = 0;
    QString lastError;
    
    // Helper methods
    void setStatus(Status newStatus);
    void makeApiRequest(const QString& endpoint, const QJsonObject& requestData, ApiCallback callback);
    void makeStreamingApiRequest(const QString& endpoint, const QJsonObject& requestData, StreamChunkCallback chunkCallback, StreamErrorCallback errorCallback);
    void handleApiReply(QNetworkReply* reply, ApiCallback callback);
    QString constructExecutablePath() const;
    QStringList buildProcessArguments() const;
    void attemptRestart();
    void resetRestartAttempts() { restartAttempts = 0; }
    
    // Validation
    bool validateConfig();
    bool isPortAvailable(int port) const;
};

Q_DECLARE_METATYPE(LlamafileManager::Status)
Q_DECLARE_METATYPE(LlamafileManager::ApiType)

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const LlamafileManager::ApiRequest& request)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "ApiRequest(" << "model=" << request.model;
    debug.nospace() << ", messages=" << request.messages;
    debug.nospace() << ", prompt=" << request.prompt;
    debug.nospace() << ", input=" << request.input;
    debug.nospace() << ", temperature=" << request.temperature;
    debug.nospace() << ", maxTokens=" << request.maxTokens;
    debug.nospace() << ", stream=" << request.stream;
    debug.nospace() << ", extraParams=" << request.extraParams;
    debug.nospace() << ')';
    return debug;
}

inline QDebug& operator<<(QDebug& debug, const LlamafileManager::ApiResponse& response)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "ApiResponse(" << "success=" << response.success;
    debug.nospace() << ", error=" << response.error;
    debug.nospace() << ", data=" << response.data;
    debug.nospace() << ", statusCode=" << response.statusCode;
    debug.nospace() << ')';
    return debug;
}

inline QDebug& operator<<(QDebug& debug, const LlamafileManager::Config& config)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "Config(" << "modelPath=" << config.modelPath;
    debug.nospace() << ", host=" << config.host;
    debug.nospace() << ", port=" << config.port;
    debug.nospace() << ", startupTimeoutMs=" << config.startupTimeoutMs;
    debug.nospace() << ", healthCheckIntervalMs=" << config.healthCheckIntervalMs;
    debug.nospace() << ", maxRestartAttempts=" << config.maxRestartAttempts;
    debug.nospace() << ", autoRestart=" << config.autoRestart;
    debug.nospace() << ", enableGpu=" << config.enableGpu;
    debug.nospace() << ", extraArgs=" << config.extraArgs;
    debug.nospace() << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM


#endif // MUDLET_LLAMAFILEMANAGER_H
