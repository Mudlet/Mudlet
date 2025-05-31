#ifndef MUDLET_LLAMAFILEMANAGER_H
#define MUDLET_LLAMAFILEMANAGER_H

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

    enum class ApiType {
        ChatCompletions,
        TextCompletions,
        Embeddings,
        Models
    };

    struct Config {
        QString modelPath;
        QString host = "127.0.0.1";
        int port = 8080;
        int startupTimeoutMs = 30000;
        int healthCheckIntervalMs = 30000;
        int maxRestartAttempts = 3;
        bool autoRestart = true;
        bool enableGpu = true;
        int contextSize = 2048;
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

#endif // MUDLET_LLAMAFILEMANAGER_H
