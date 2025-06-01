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

#include "LlamaFileManager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include <QNetworkProxyFactory>
#include <QTcpSocket>
#include <QJsonParseError>

LlamafileManager::LlamafileManager(QObject* parent)
    : QObject(parent)
    , process(std::make_unique<QProcess>(this))
    , healthCheckTimer(std::make_unique<QTimer>(this))
    , networkManager(std::make_unique<QNetworkAccessManager>(this))
{
    // Configure process
    connect(process.get(), &QProcess::started, this, &LlamafileManager::onProcessStarted);
    connect(process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LlamafileManager::onProcessFinished);
    connect(process.get(), &QProcess::errorOccurred, this, &LlamafileManager::onProcessError);
    connect(process.get(), &QProcess::stateChanged, this, &LlamafileManager::onProcessStateChanged);
    
    // Configure health check timer
    healthCheckTimer->setSingleShot(false);
    connect(healthCheckTimer.get(), &QTimer::timeout, this, &LlamafileManager::performHealthCheck);
    
    // Configure network manager
    networkManager->setTransferTimeout(10000); // 10 second timeout
}

LlamafileManager::~LlamafileManager() {
    if (isRunning()) {
        stop();
    }
}

bool LlamafileManager::start(const Config& newConfig) {
    if (currentStatus == Status::Starting || currentStatus == Status::Running) {
        qDebug() << "LlamafileManager: Already starting or running";
        return currentStatus == Status::Running;
    }
    
    config = newConfig;
    
    if (!validateConfig()) {
        qDebug() << "LlamafileManager: Invalid configuration: " << lastError;
        setStatus(Status::Error);
        return false;
    }
    
    setStatus(Status::Starting);
    
    const QString executable = qsl("/bin/sh");
    const QStringList args = buildProcessArguments();
    
    qDebug().noquote() << "Starting llamafile:" << executable << args.join(" ");
    
    // Set working directory to the model's directory
    QFileInfo fileInfo(config.modelPath);
    if (fileInfo.exists()) {
        process->setWorkingDirectory(fileInfo.absolutePath());
    }
    
    process->start(executable, args);
    
    // Wait for startup with timeout
    if (!process->waitForStarted(config.startupTimeoutMs)) {
        const QString error = QString("Failed to start llamafile: %1").arg(process->errorString());
        lastError = error;
        setStatus(Status::Error);
        emit processError(error);
        return false;
    }
    
    return true;
}

void LlamafileManager::stop() {
    if (currentStatus == Status::Stopped || currentStatus == Status::Stopping) {
        return;
    }
    
    setStatus(Status::Stopping);
    healthCheckTimer->stop();
    healthy = false;
    
    if (process && process->state() != QProcess::NotRunning) {
        // Try graceful termination first
        process->terminate();
        
        // Wait up to 5 seconds for graceful shutdown
        if (!process->waitForFinished(5000)) {
            qDebug() << "LlamafileManager: Graceful shutdown failed, killing process";
            process->kill();
            process->waitForFinished(3000);
        }
    }
    
    setStatus(Status::Stopped);
    emit processStopped();
}

std::optional<qint64> LlamafileManager::processId() const noexcept {
    if (process && process->state() == QProcess::Running) {
        return process->processId();
    }
    return std::nullopt;
}

void LlamafileManager::chatCompletion(const ApiRequest& request, ApiCallback callback) {
    if (!isRunning()) {
        callback({false, "Llamafile not running", {}, 0});
        return;
    }
    
    QJsonObject requestData;
    requestData["model"] = request.model;
    requestData["messages"] = request.messages;
    requestData["temperature"] = request.temperature;
    requestData["max_tokens"] = request.maxTokens;
    requestData["stream"] = request.stream;
    
    // Merge extra parameters
    for (auto it = request.extraParams.begin(); it != request.extraParams.end(); ++it) {
        requestData[it.key()] = it.value();
    }
    
    makeApiRequest("/v1/chat/completions", requestData, std::move(callback));
}

void LlamafileManager::textCompletion(const ApiRequest& request, ApiCallback callback) {
    if (!isRunning()) {
        callback({false, "Llamafile not running", {}, 0});
        return;
    }
    
    QJsonObject requestData;
    requestData["prompt"] = request.prompt;
    requestData["temperature"] = request.temperature;
    if (request.maxTokens > 0) {
        requestData["n_predict"] = request.maxTokens;
    }
    requestData["stream"] = request.stream;
    
    // Merge extra parameters
    for (auto it = request.extraParams.begin(); it != request.extraParams.end(); ++it) {
        requestData[it.key()] = it.value();
    }
    
    makeApiRequest("/completion", requestData, std::move(callback));
}

void LlamafileManager::embeddings(const ApiRequest& request, ApiCallback callback) {
    if (!isRunning()) {
        callback({false, "Llamafile not running", {}, 0});
        return;
    }
    
    QJsonObject requestData;
    requestData["model"] = request.model;
    
    if (request.input.size() == 1) {
        requestData["input"] = request.input.first();
    } else {
        QJsonArray inputArray;
        for (const QString& text : request.input) {
            inputArray.append(text);
        }
        requestData["input"] = inputArray;
    }
    
    makeApiRequest("/v1/embeddings", requestData, std::move(callback));
}

void LlamafileManager::getModels(ApiCallback callback) {
    if (!isRunning()) {
        callback({false, "Llamafile not running", {}, 0});
        return;
    }
    
    makeApiRequest("/v1/models", {}, std::move(callback));
}

void LlamafileManager::enableHealthCheck(bool enable) {
    if (enable && isRunning()) {
        healthCheckTimer->start(config.healthCheckIntervalMs);
    } else {
        healthCheckTimer->stop();
    }
}

bool LlamafileManager::isLlamafileExecutable(const QString& path) {
    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return false;
    }
    
#if defined(Q_OS_WINDOWS)
    // Check if it's executable
    if (!info.isExecutable()) {
        return false;
    }
#endif
    
    // Basic heuristics for llamafile detection
    const QString fileName = info.fileName().toLower();
    return fileName.endsWith(".llamafile");
}

QString LlamafileManager::findLlamafileExecutable(const QStringList& searchPaths) {
    QStringList paths = searchPaths;
    
    // Add default search paths
    if (paths.isEmpty()) {
        paths << QCoreApplication::applicationDirPath()
              << QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
              << QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
              << "/usr/local/bin"
              << "/opt/llamafile";
    }
    
    for (const QString& path : paths) {
        QDir dir(path);
        if (!dir.exists()) continue;
        
        const QStringList filters{"*.llamafile"};
        const auto entries = dir.entryInfoList(filters, QDir::Files | QDir::Executable);
        
        for (const QFileInfo& entry : entries) {
            if (isLlamafileExecutable(entry.absoluteFilePath())) {
                return entry.absoluteFilePath();
            }
        }
    }
    
    return {};
}

QUrl LlamafileManager::apiBaseUrl() const {
    return QUrl(QString("http://%1:%2").arg(config.host).arg(config.port));
}

// Private slots
void LlamafileManager::onProcessStarted() {
    qDebug() << "LlamafileManager: Process started with PID" << process->processId();
    
    // Wait a moment for the server to initialize before declaring it running
    QTimer::singleShot(2000, this, [this]() {
        if (process && process->state() == QProcess::Running) {
            setStatus(Status::Running);
            resetRestartAttempts();
            emit processStarted();
            
            // Start health checking
            enableHealthCheck(true);
            
            // Perform initial health check
            QTimer::singleShot(1000, this, &LlamafileManager::performHealthCheck);
        }
    });
}

void LlamafileManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug() << "LlamafileManager: Process finished with exit code" << exitCode 
             << "status" << (exitStatus == QProcess::NormalExit ? "normal" : "crashed");
    
    // Capture any remaining output from the process
    QString stdoutOutput = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    QString stderrOutput = QString::fromUtf8(process->readAllStandardError()).trimmed();
    
    // Get last few lines of output for context
    auto getLastLines = [](const QString& text, int maxLines = 5) -> QString {
        if (text.isEmpty()) return QString();
        
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        if (lines.size() <= maxLines) {
            return text;
        }
        
        QStringList lastLines = lines.mid(lines.size() - maxLines);
        return lastLines.join('\n');
    };
    
    QString recentStdout = getLastLines(stdoutOutput);
    QString recentStderr = getLastLines(stderrOutput);
    
    healthCheckTimer->stop();
    healthy = false;
    
    if (currentStatus != Status::Stopping) {
        if (config.autoRestart && restartAttempts < config.maxRestartAttempts) {
            qDebug() << "LlamafileManager: Attempting restart" << (restartAttempts + 1) 
                     << "of" << config.maxRestartAttempts;
            
            // Log the output for debugging restart scenarios
            if (!recentStdout.isEmpty()) {
                qDebug() << "LlamafileManager: Recent stdout:" << recentStdout;
            }
            if (!recentStderr.isEmpty()) {
                qDebug() << "LlamafileManager: Recent stderr:" << recentStderr;
            }
            
            setStatus(Status::Stopped);
            attemptRestart();
        } else {
            QString errorMsg = QString("Process exited unexpectedly (code: %1)").arg(exitCode);
            
            // Append recent output to error message
            if (!recentStderr.isEmpty()) {
                errorMsg += QString("\nRecent stderr:\n%1").arg(recentStderr);
            }
            if (!recentStdout.isEmpty()) {
                errorMsg += QString("\nRecent stdout:\n%1").arg(recentStdout);
            }
            
            setStatus(Status::Error);
            lastError = errorMsg;
            emit processError(lastError);
        }
    } else {
        setStatus(Status::Stopped);
    }
}

void LlamafileManager::onProcessError(QProcess::ProcessError error) {
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start: " + process->errorString();
            break;
        case QProcess::Crashed:
            errorString = "Process crashed";
            break;
        case QProcess::Timedout:
            errorString = "Process timed out";
            break;
        case QProcess::WriteError:
            errorString = "Write error: " + process->errorString();
            break;
        case QProcess::ReadError:
            errorString = "Read error: " + process->errorString();
            break;
        default:
            errorString = "Unknown error: " + process->errorString();
            break;
    }
    
    qDebug() << "LlamafileManager: Process error:" << errorString;
    lastError = errorString;
    
    if (currentStatus == Status::Starting) {
        setStatus(Status::Error);
        emit processError(errorString);
    }
}

void LlamafileManager::onProcessStateChanged(QProcess::ProcessState newState) {
    qDebug() << "LlamafileManager: Process state changed to" << newState;
}

void LlamafileManager::performHealthCheck() {
    if (!isRunning()) {
        return;
    }
    
    const QUrl url = apiBaseUrl().resolved(QUrl("/v1/models"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer no-key");
    
    auto* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &LlamafileManager::onHealthCheckReply);
    
    // Set a property to identify this as a health check
    reply->setProperty("isHealthCheck", true);
}

void LlamafileManager::onHealthCheckReply() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    const bool wasHealthy = healthy;
    
    if (reply->error() == QNetworkReply::NoError) {
        healthy = true;
        if (!wasHealthy) {
            qDebug() << "LlamafileManager: Health check passed";
            emit healthCheckPassed();
        }
    } else {
        healthy = false;
        const QString reason = QString("HTTP %1: %2")
                              .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                              .arg(reply->errorString());
        
        qDebug() << "LlamafileManager: Health check failed:" << reason;
        setStatus(Status::Unhealthy);
        emit healthCheckFailed(reason);
        
        // Consider restarting if health checks consistently fail
        if (config.autoRestart && !wasHealthy) {
            QTimer::singleShot(5000, this, [this]() {
                if (!healthy && config.autoRestart) {
                    attemptRestart();
                }
            });
        }
    }
}

// Private helper methods
void LlamafileManager::setStatus(Status newStatus) {
    if (currentStatus != newStatus) {
        const Status oldStatus = currentStatus;
        currentStatus = newStatus;
        
        qDebug() << "LlamafileManager: Status changed from" << oldStatus << "to" << newStatus;
        
        emit statusChanged(newStatus, oldStatus);
    }
}

void LlamafileManager::makeApiRequest(const QString& endpoint, const QJsonObject& requestData, ApiCallback callback) {
    const QUrl url = apiBaseUrl().resolved(QUrl(endpoint));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer no-key");
    
    QNetworkReply* reply;
    if (requestData.isEmpty()) {
        reply = networkManager->get(request);
    } else {
        const QByteArray data = QJsonDocument(requestData).toJson(QJsonDocument::Compact);
        reply = networkManager->post(request, data);
    }
    
    connect(reply, &QNetworkReply::finished, [this, reply, callback = std::move(callback)]() {
        handleApiReply(reply, callback);
    });
}

void LlamafileManager::handleApiReply(QNetworkReply* reply, ApiCallback callback) {
    reply->deleteLater();
    
    ApiResponse response;
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        
        if (parseError.error == QJsonParseError::NoError) {
            response.success = true;
            response.data = doc.object();
        } else {
            response.success = false;
            response.error = "JSON parse error: " + parseError.errorString();
        }
    } else {
        response.success = false;
        response.error = reply->errorString();
    }
    
    callback(response);
}

QString LlamafileManager::constructExecutablePath() const {
    QString executable;

#if defined(Q_OS_LINUX)
    return qsl("/bin/sh");
#endif

    executable = executable + config.modelPath;
    
    return executable;
}

QStringList LlamafileManager::buildProcessArguments() const {
    QStringList args;
    
    // Basic server arguments
    args << config.modelPath;
    args << "--server";
    // v2 doesn't work: https://github.com/Mozilla-Ocho/llamafile/issues/695#issuecomment-2926499556
    // args << "--v2";
    // args << qsl("-l %1:%2").arg(config.host).arg(config.port);
    args << "--host";
    args << config.host;
    args << "--port";
    args << QString::number(config.port);

    // required for v1 only, incompatible with v2
    args << "--nobrowser";
    
    // GPU settings
    if (config.enableGpu) {
        // doesn't work with v1
        // args << "--gpu auto";
        // args << "--ngl 999"
    }
    
    // Add any extra arguments
    args << config.extraArgs;
    
    return args;
}

void LlamafileManager::attemptRestart() {
    if (restartAttempts >= config.maxRestartAttempts) {
        setStatus(Status::Error);
        lastError = QString("Max restart attempts (%1) exceeded").arg(config.maxRestartAttempts);
        emit processError(lastError);
        return;
    }
    
    ++restartAttempts;
    qDebug() << "LlamafileManager: Restart attempt" << restartAttempts;
    
    // Wait a bit before restarting
    QTimer::singleShot(2000, this, [this]() {
        start(config);
    });
}

bool LlamafileManager::validateConfig() {
    if (config.modelPath.isEmpty()) {
        lastError = "Model path is empty";
        return false;
    }
    
    const QFileInfo fileInfo(config.modelPath);
    if (!fileInfo.exists()) {
        lastError = "Model file does not exist: " + config.modelPath;
        return false;
    }
    
    if (!fileInfo.isExecutable()) {
        lastError = "Model file is not executable: " + config.modelPath;
        return false;
    }
    
    if (config.port <= 0 || config.port > 65535) {
        lastError = QString("Invalid port: %1").arg(config.port);
        return false;
    }
    
    return true;
}

bool LlamafileManager::isPortAvailable(int port) const {
    QTcpSocket socket;
    socket.connectToHost(config.host, port);
    const bool available = !socket.waitForConnected(100);
    socket.disconnectFromHost();
    return available;
}

void LlamafileManager::textCompletionStream(const ApiRequest& request, StreamChunkCallback chunkCallback, StreamErrorCallback errorCallback) {
    if (!isRunning()) {
        errorCallback("Llamafile not running");
        return;
    }
    
    QJsonObject requestData;
    requestData["prompt"] = request.prompt;
    requestData["temperature"] = request.temperature;
    if (request.maxTokens > 0) {
        requestData["n_predict"] = request.maxTokens;
    }
    requestData["stream"] = true; // Force streaming
    
    // Merge extra parameters
    for (auto it = request.extraParams.begin(); it != request.extraParams.end(); ++it) {
        requestData[it.key()] = it.value();
    }
    
    makeStreamingApiRequest("/completion", requestData, chunkCallback, errorCallback);
}

void LlamafileManager::makeStreamingApiRequest(const QString& endpoint, const QJsonObject& requestData, StreamChunkCallback chunkCallback, StreamErrorCallback errorCallback) {
    const QUrl url = apiBaseUrl().resolved(QUrl(endpoint));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer no-key");
    
    const QByteArray data = QJsonDocument(requestData).toJson(QJsonDocument::Compact);
    auto* reply = networkManager->post(request, data);
    
    // Handle streaming response
    connect(reply, &QNetworkReply::readyRead, [reply, chunkCallback]() {
        QByteArray data = reply->readAll();
        QString chunk = QString::fromUtf8(data);
        
        // Parse Server-Sent Events format
        QStringList lines = chunk.split('\n');
        for (const QString& line : lines) {
            if (line.startsWith("data: ")) {
                QString jsonData = line.mid(6); // Remove "data: " prefix
                if (jsonData == "[DONE]") {
                    chunkCallback("", true); // Signal completion
                    return;
                }
                
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
                if (parseError.error == QJsonParseError::NoError) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("content")) {
                        chunkCallback(obj["content"].toString(), false);
                    }
                }
            }
        }
    });
    
    connect(reply, &QNetworkReply::finished, [reply, chunkCallback, errorCallback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            errorCallback(reply->errorString());
        } else {
            chunkCallback("", true); // Signal completion
        }
    });
}
