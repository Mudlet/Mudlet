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

#include "SecureStringUtils.h"

#include "utils.h"

#include "pre_guard.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVersionNumber>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include "../3rdparty/qtkeychain/keychain.h"
#else
#include <qt6keychain/keychain.h>
#endif
#include "post_guard.h"

bool SecureStringUtils::isEncryptedFormat(const QString& text)
{
    if (text.isEmpty()) {
        return false;
    }
    
    // Quick length check - encrypted strings are much longer due to overhead
    if (text.length() < (MIN_ENCRYPTED_SIZE * 4 / 3)) { // Base64 encoding overhead
        return false;
    }
    
    // Check if it's valid Base64
    QRegularExpression base64Regex(qsl("^[A-Za-z0-9+/]*={0,2}$"));
    if (!base64Regex.match(text).hasMatch()) {
        return false;
    }
    
    try {
        // Try to decode and check structure
        QByteArray decoded = QByteArray::fromBase64(text.toLatin1());

        if (decoded.size() < MIN_ENCRYPTED_SIZE) {
            return false;
        }
        
        // Check version byte
        quint8 version = static_cast<quint8>(decoded[0]);
        return (version == ENCRYPTION_VERSION);
        
    } catch (...) {
        return false;
    }
}

void SecureStringUtils::secureStringClear(QString& str)
{
    // Overwrite the string's data with zeros
    if (!str.isEmpty()) {
        str.fill(QChar('\0'));
        str.clear();
    }
}

void SecureStringUtils::secureByteArrayClear(QByteArray& array)
{
    // Overwrite the array's data with zeros
    if (!array.isEmpty()) {
        array.fill('\0');
        array.clear();
    }
}

QByteArray SecureStringUtils::generateKey(const QByteArray& password, const QByteArray& salt, int iterations)
{
    // Use iterative SHA-256 hashing to implement PBKDF2-like key derivation
    QByteArray derivedKey = password + salt;
    
    for (int i = 0; i < iterations; ++i) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(derivedKey);
        hash.addData(salt);
        derivedKey = hash.result();
    }
    
    // Ensure we have exactly KEY_SIZE bytes
    if (derivedKey.size() > KEY_SIZE) {
        derivedKey = derivedKey.left(KEY_SIZE);
    } else if (derivedKey.size() < KEY_SIZE) {
        // Extend key if needed by hashing again
        while (derivedKey.size() < KEY_SIZE) {
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(derivedKey);
            derivedKey.append(hash.result());
        }

        derivedKey = derivedKey.left(KEY_SIZE);
    }
    
    return derivedKey;
}

QByteArray SecureStringUtils::generateSalt()
{
    // Generate a random 16-byte salt
    QByteArray salt;
    salt.resize(SALT_SIZE);
    
    QRandomGenerator* rng = QRandomGenerator::system();

    for (int i = 0; i < SALT_SIZE; ++i) {
        salt[i] = static_cast<char>(rng->bounded(256));
    }
    
    return salt;
}

QByteArray SecureStringUtils::generateNonce()
{
    // Generate a random 16-byte nonce
    QByteArray nonce;

    nonce.resize(NONCE_SIZE);
    
    QRandomGenerator* rng = QRandomGenerator::system();

    for (int i = 0; i < NONCE_SIZE; ++i) {
        nonce[i] = static_cast<char>(rng->bounded(256));
    }
    
    return nonce;
}

QByteArray SecureStringUtils::generateKeystream(const QByteArray& key, const QByteArray& nonce, int length)
{
    // Generate a keystream using hash-based approach (ChaCha20-inspired)
    // This provides cryptographically secure stream cipher encryption
    QByteArray keystream;
    keystream.reserve(length);
    
    int blocks = (length + 31) / 32; // 32 bytes per hash output
    
    for (int block = 0; block < blocks; ++block) {
        QCryptographicHash hash(QCryptographicHash::Sha256);

        hash.addData(key);
        hash.addData(nonce);
        
        // Add block counter as 4-byte little-endian integer
        QByteArray blockCounter;

        blockCounter.resize(4);
        blockCounter[0] = static_cast<char>(block & 0xFF);
        blockCounter[1] = static_cast<char>((block >> 8) & 0xFF);
        blockCounter[2] = static_cast<char>((block >> 16) & 0xFF);
        blockCounter[3] = static_cast<char>((block >> 24) & 0xFF);
        hash.addData(blockCounter);
        
        keystream.append(hash.result());
    }
    
    // Truncate to exact length needed
    if (keystream.size() > length) {
        keystream = keystream.left(length);
    }
    
    return keystream;
}

QString SecureStringUtils::encryptStringForProfile(const QString& plaintext, const QString& profileName)
{
    if (plaintext.isEmpty() || profileName.isEmpty()) {
        return QString();
    }
    
    try {
        // Convert to UTF-8 bytes
        QByteArray plaintextBytes = plaintext.toUtf8();
        
        // Generate random salt and nonce for this encryption
        QByteArray salt = generateSalt();
        QByteArray nonce = generateNonce();
        
        // Get profile-specific encryption key
        QByteArray profileKey = getProfileEncryptionKey(profileName);
        QByteArray key = generateKey(profileKey, salt);
        
        // Create keystream using hash-based approach
        QByteArray keystream = generateKeystream(key, nonce, plaintextBytes.size());
        
        // Encrypt by XORing with keystream
        QByteArray encrypted;

        encrypted.resize(plaintextBytes.size());

        for (int i = 0; i < plaintextBytes.size(); ++i) {
            encrypted[i] = plaintextBytes[i] ^ keystream[i];
        }
        
        // Build the final format: [VERSION:1][SALT:16][NONCE:16][ENCRYPTED_DATA]
        QByteArray result;

        result.append(static_cast<char>(ENCRYPTION_VERSION));
        result.append(salt);
        result.append(nonce);
        result.append(encrypted);
        
        // Encode as Base64 for safe storage in XML
        QString base64Result = result.toBase64();
        
        // Clear sensitive data
        secureByteArrayClear(plaintextBytes);
        secureByteArrayClear(profileKey);
        secureByteArrayClear(key);
        secureByteArrayClear(keystream);
        secureByteArrayClear(salt);
        secureByteArrayClear(nonce);
        secureByteArrayClear(encrypted);
        secureByteArrayClear(result);
        
        return base64Result;
        
    } catch (...) {
        // If encryption fails for any reason, return empty string
        return QString();
    }
}

QString SecureStringUtils::decryptStringForProfile(const QString& ciphertext, const QString& profileName)
{
    if (ciphertext.isEmpty() || profileName.isEmpty()) {
        return QString();
    }
    
    try {
        // Decode from Base64
        QByteArray encrypted = QByteArray::fromBase64(ciphertext.toLatin1());

        if (encrypted.size() < MIN_ENCRYPTED_SIZE) {
            return QString(); // Invalid format
        }
        
        // Extract version
        quint8 version = static_cast<quint8>(encrypted[0]);

        if (version != ENCRYPTION_VERSION) {
            return QString(); // Unsupported version
        }
        
        // Extract salt (bytes 1-16)
        QByteArray salt = encrypted.mid(1, SALT_SIZE);
        
        // Extract nonce (bytes 17-32)
        QByteArray nonce = encrypted.mid(1 + SALT_SIZE, NONCE_SIZE);
        
        // Extract encrypted data (bytes 33+)
        QByteArray encryptedData = encrypted.mid(1 + SALT_SIZE + NONCE_SIZE);
        
        // Get profile-specific encryption key
        QByteArray profileKey = getProfileEncryptionKey(profileName);
        QByteArray key = generateKey(profileKey, salt);
        
        // Create keystream using hash-based approach
        QByteArray keystream = generateKeystream(key, nonce, encryptedData.size());
        
        // Decrypt by XORing with keystream
        QByteArray decrypted;
        decrypted.resize(encryptedData.size());

        for (int i = 0; i < encryptedData.size(); ++i) {
            decrypted[i] = encryptedData[i] ^ keystream[i];
        }
        
        // Convert back to QString
        QString result = QString::fromUtf8(decrypted);
        
        // Clear sensitive data
        secureByteArrayClear(encrypted);
        secureByteArrayClear(salt);
        secureByteArrayClear(nonce);
        secureByteArrayClear(encryptedData);
        secureByteArrayClear(profileKey);
        secureByteArrayClear(key);
        secureByteArrayClear(keystream);
        secureByteArrayClear(decrypted);
        
        return result;
    } catch (...) {
        // If decryption fails for any reason, return empty string
        return QString();
    }
}

QByteArray SecureStringUtils::getProfileEncryptionKey(const QString& profileName)
{
    QByteArray existingKey;
    
    // Skip keychain in test environment
    if (!isTestEnvironment()) {
        // Try to load existing key from secure storage first
        auto *job = new QKeychain::ReadPasswordJob(qsl("Mudlet profile encryption"));

        job->setAutoDelete(false);
        job->setInsecureFallback(false);
        job->setKey(profileName);
        
        // Create a blocking call using QEventLoop
        QEventLoop loop;
        
        QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [&](QKeychain::Job* task) {
            if (!task->error()) {
                auto readJob = static_cast<QKeychain::ReadPasswordJob*>(task);
                existingKey = QByteArray::fromBase64(readJob->textData().toLatin1());
            }
            loop.quit();
        });
        
        job->start();
        loop.exec();
        job->deleteLater();
        
        // If we found an existing key and it's the right size, use it
        if (existingKey.size() == KEY_SIZE) {
            return existingKey;
        }
    }
    
    // If keychain failed or we're in test mode, try to load from profile directory (portable mode)
    QByteArray fileKey = loadEncryptionKeyFromFile(profileName);

    if (fileKey.size() == KEY_SIZE) {
        return fileKey;
    }
    
    // Generate a new random key
    QByteArray newKey;

    newKey.resize(KEY_SIZE);
    
    QRandomGenerator* rng = QRandomGenerator::system();

    for (int i = 0; i < KEY_SIZE; ++i) {
        newKey[i] = static_cast<char>(rng->bounded(256));
    }
    
    // Try to store the new key in secure storage first (skip in test mode)
    if (!isTestEnvironment() && storeProfileEncryptionKey(profileName, newKey)) {
        return newKey;
    }
    
    // If secure storage failed or we're in test mode, store in profile directory (portable mode)
    if (storeEncryptionKeyToFile(profileName, newKey)) {
        return newKey;
    }
    
    // Final fallback to deterministic key if all else fails
    // This ensures compatibility when profile directory is read-only
    QCryptographicHash hash(QCryptographicHash::Sha256);

    hash.addData(qsl("Mudlet").toUtf8());
    hash.addData(profileName.toUtf8());
    hash.addData(qsl("MudletProfileEncryption2025").toUtf8());
    
    return hash.result();
}

bool SecureStringUtils::storeProfileEncryptionKey(const QString& profileName, const QByteArray& key)
{
    // Skip keychain in test environment
    if (isTestEnvironment()) {
        return false; // Let caller fall back to file storage
    }
    
    auto *job = new QKeychain::WritePasswordJob(qsl("Mudlet profile encryption"));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    job->setTextData(key.toBase64());
    
    // Create a blocking call using QEventLoop
    QEventLoop loop;
    bool success = false;
    
    QObject::connect(job, &QKeychain::WritePasswordJob::finished, [&](QKeychain::Job* task) {
        success = !task->error();

        if (task->error()) {
            qDebug().nospace().noquote() << "SecureStringUtils::storeProfileEncryptionKey() WARNING - could not store encryption key for profile \"" 
                                         << profileName << "\", error: " << task->errorString() << ". Falling back to legacy key derivation.";
        }

        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return success;
}

QByteArray SecureStringUtils::loadEncryptionKeyFromFile(const QString& profileName)
{
    // Build path manually to avoid circular dependencies
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString keyFilePath = QString("%1/profiles/%2/encryption_key").arg(configPath, profileName);
    
    QFile file(keyFilePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray(); // File doesn't exist or can't be read
    }
    
    QDataStream ifs(&file);
    // Use compatible data stream format
    ifs.setVersion(QDataStream::Qt_5_12);
    
    QString base64Key;

    ifs >> base64Key;
    file.close();
    
    if (base64Key.isEmpty()) {
        return QByteArray();
    }
    
    QByteArray key = QByteArray::fromBase64(base64Key.toLatin1());
    return (key.size() == KEY_SIZE) ? key : QByteArray();
}

bool SecureStringUtils::storeEncryptionKeyToFile(const QString& profileName, const QByteArray& key)
{
    if (key.size() != KEY_SIZE) {
        return false;
    }
    
    // Build path manually to avoid circular dependencies
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString profileDir = QString("%1/profiles/%2").arg(configPath, profileName);
    QString keyFilePath = QString("%1/encryption_key").arg(profileDir);
    
    // Ensure profile directory exists
    QDir dir;

    if (!dir.mkpath(profileDir)) {
        qDebug().nospace().noquote() << "SecureStringUtils::storeEncryptionKeyToFile() WARNING - could not create profile directory for \"" 
                                     << profileName << "\". Falling back to deterministic key derivation.";
        return false;
    }
    
    QSaveFile file(keyFilePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        qDebug().nospace().noquote() << "SecureStringUtils::storeEncryptionKeyToFile() WARNING - could not create encryption key file for profile \"" 
                                     << profileName << "\", error: " << file.errorString() << ". Falling back to deterministic key derivation.";
        return false;
    }
    
    QDataStream ofs(&file);
    // Use compatible data stream format
    ofs.setVersion(QDataStream::Qt_5_12);
    
    QString base64Key = key.toBase64();
    ofs << base64Key;
    
    if (!file.commit()) {
        qDebug().nospace().noquote() << "SecureStringUtils::storeEncryptionKeyToFile() WARNING - could not save encryption key file for profile \"" 
                                     << profileName << "\", error: " << file.errorString() << ". Falling back to deterministic key derivation.";
        return false;
    }
    
    return true;
}

bool SecureStringUtils::isTestEnvironment()
{
    // Check if we're running in a test environment
    // This prevents keychain access during automated testing
    
    // Check various indicators that we're in a test environment
    QString appName = QCoreApplication::applicationName();
    QStringList args = QCoreApplication::arguments();
    
    return qEnvironmentVariableIsSet("MUDLET_TEST_MODE") ||
           appName.contains("Test", Qt::CaseInsensitive) ||
           args.first().contains("Test", Qt::CaseInsensitive);
}
