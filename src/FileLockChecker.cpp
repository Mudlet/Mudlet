#include "FileLockChecker.h"
#include <QTimer>
#include <QDebug>
#include <windows.h>

FileLockChecker::FileLockChecker(const QString& filePath, QObject* parent)
: QObject(parent), m_filePath(filePath), m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &FileLockChecker::checkFileLock);
}

void FileLockChecker::startChecking(int interval)
{
    m_timer->start(interval);
}

void FileLockChecker::checkFileLock()
{
    if (isFileWritable(m_filePath))
    {
        m_timer->stop();
        emit fileAvailable();
        qDebug() << "File is now available for writing!";
    }
    else
    {
        qDebug() << "File is still locked...";
    }
}

bool FileLockChecker::isFileWritable(const QString& filePath)
{
    HANDLE fileHandle = CreateFileW(
            reinterpret_cast<LPCWSTR>(filePath.utf16()),
            GENERIC_WRITE,
            0, // No sharing
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    CloseHandle(fileHandle);
    return true;
}
