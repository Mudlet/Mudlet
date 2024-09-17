#ifndef FILELOCKCHECKER_H
#define FILELOCKCHECKER_H

#include <QObject>
#include <QString>

class QTimer;

class FileLockChecker : public QObject
{
    Q_OBJECT

public:
    explicit FileLockChecker(const QString& filePath, QObject* parent = nullptr);
    void startChecking(int interval = 1000);

private slots:
    void checkFileLock();

signals:
    void fileAvailable();

private:
    bool isFileWritable(const QString& filePath);

    QString m_filePath;
    QTimer* m_timer;
};

#endif // FILELOCKCHECKER_H
