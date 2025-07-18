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

#include "PasswordManager.h"
#include "SecureStringUtils.h"
#include "utils.h"

#include "pre_guard.h"
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QObject>
#include <QStandardPaths>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include "../3rdparty/qtkeychain/keychain.h"
#else
#include <qt6keychain/keychain.h>
#endif
#include "post_guard.h"

bool PasswordManager::storePassword(const QString& profileName, const QString& key, const QString& password)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return false;
    }
    
    // Empty password is valid - it means "no password"
    if (password.isEmpty()) {
        return removePassword(profileName, key);
    }
    
    // Try QtKeychain first
    if (storeInKeychain(profileName, key, password)) {
        // Also remove any encrypted fallback file if it exists
        removeEncrypted(profileName, key);
        return true;
    }
    
    // Fallback to encrypted storage for portable mode
    return storeEncrypted(profileName, key, password);
}

QString PasswordManager::retrievePassword(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return QString();
    }
    
    // Try QtKeychain first
    QString password = retrieveFromKeychain(profileName, key);
    if (!password.isNull()) {
        return password;
    }
    
    // Fallback to encrypted storage
    return retrieveEncrypted(profileName, key);
}

bool PasswordManager::removePassword(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return false;
    }
    
    bool keychainRemoved = removeFromKeychain(profileName, key);
    bool encryptedRemoved = removeEncrypted(profileName, key);
    
    // Success if at least one method succeeded (or both failed because nothing was stored)
    return keychainRemoved || encryptedRemoved;
}

bool PasswordManager::isKeychainAvailable()
{
    // Test by trying to read a non-existent key
    auto *job = new QKeychain::ReadPasswordJob(qsl("MudletKeychainTest"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(qsl("NonExistentTestKey"));
    
    QEventLoop loop;
    bool available = false;
    
    QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [&](QKeychain::Job* task) {
        // If error is "Entry not found", keychain is working
        // If error is "No such keychain service" or similar, keychain is not available
        available = (task->error() == QKeychain::NoError || 
                    task->error() == QKeychain::EntryNotFound);
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return available;
}

bool PasswordManager::storeInKeychain(const QString& profileName, const QString& key, const QString& password)
{
    auto *job = new QKeychain::WritePasswordJob(generateServiceName(profileName, key));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    job->setTextData(password);
    
    QEventLoop loop;
    bool success = false;
    
    QObject::connect(job, &QKeychain::WritePasswordJob::finished, [&](QKeychain::Job* task) {
        success = !task->error();
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return success;
}

QString PasswordManager::retrieveFromKeychain(const QString& profileName, const QString& key)
{
    auto *job = new QKeychain::ReadPasswordJob(generateServiceName(profileName, key));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    
    QEventLoop loop;
    QString password;
    
    QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [&](QKeychain::Job* task) {
        if (!task->error()) {
            auto readJob = static_cast<QKeychain::ReadPasswordJob*>(task);
            password = readJob->textData();
        }
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return password;
}

bool PasswordManager::removeFromKeychain(const QString& profileName, const QString& key)
{
    auto *job = new QKeychain::DeletePasswordJob(generateServiceName(profileName, key));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    
    QEventLoop loop;
    bool success = false;
    
    QObject::connect(job, &QKeychain::DeletePasswordJob::finished, [&](QKeychain::Job* task) {
        success = !task->error() || task->error() == QKeychain::EntryNotFound;
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return success;
}

bool PasswordManager::storeEncrypted(const QString& profileName, const QString& key, const QString& password)
{
    QString filePath = generateFilePath(profileName, key);
    
    // Ensure directory exists
    QDir dir = QDir(filePath).absolutePath();
    dir.cdUp();
    if (!dir.mkpath(dir.absolutePath())) {
        return false;
    }
    
    // Encrypt password using SecureStringUtils
    QString encrypted = SecureStringUtils::encryptStringForProfile(password, profileName);
    if (encrypted.isEmpty()) {
        return false;
    }
    
    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    file.write(encrypted.toUtf8());
    file.close();
    
    return true;
}

QString PasswordManager::retrieveEncrypted(const QString& profileName, const QString& key)
{
    QString filePath = generateFilePath(profileName, key);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QString encrypted = QString::fromUtf8(file.readAll());
    file.close();
    
    if (encrypted.isEmpty()) {
        return QString();
    }
    
    // Decrypt using SecureStringUtils
    return SecureStringUtils::decryptStringForProfile(encrypted, profileName);
}

bool PasswordManager::removeEncrypted(const QString& profileName, const QString& key)
{
    QString filePath = generateFilePath(profileName, key);
    return QFile::remove(filePath);
}

QString PasswordManager::generateServiceName(const QString& profileName, const QString& key)
{
    return QString("Mudlet-%1-%2").arg(profileName, key);
}

QString PasswordManager::generateFilePath(const QString& profileName, const QString& key)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QString("%1/profiles/%2/passwords/%3").arg(configPath, profileName, key);
}
