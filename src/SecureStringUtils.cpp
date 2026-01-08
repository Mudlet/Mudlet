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

#include <atomic>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVersionNumber>
// Qt includes for encryption and SSL availability check
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif

QString SecureStringUtils::getSSLBackendInfo()
{
    QStringList info;

#ifndef QT_NO_SSL
    // Check Qt's SSL backend
    if (QSslSocket::isProtocolSupported(QSsl::TlsV1_2)) {
        info << "Qt SSL support: Available";
        info << QString("SSL backend library: %1").arg(QSslSocket::sslLibraryBuildVersionString());
        info << QString("Active backend: %1").arg(QSslSocket::activeBackend());

        // Check supported protocols
        QStringList protocols;
        if (QSslSocket::isProtocolSupported(QSsl::TlsV1_2)) protocols << "TLS 1.2";
        if (QSslSocket::isProtocolSupported(QSsl::TlsV1_3)) protocols << "TLS 1.3";
        info << QString("Supported protocols: %1").arg(protocols.join(", "));

    } else {
        info << "Qt SSL support: Not available";
    }
#else
    info << "Qt SSL support: Compiled without SSL";
#endif

    return info.join("\n");
}

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

    // Try to decode and check structure
    QByteArray decoded = QByteArray::fromBase64(text.toLatin1());

    if (decoded.size() < MIN_ENCRYPTED_SIZE) {
        return false;
    }

    // Check version byte - only support current version
    quint8 version = static_cast<quint8>(decoded[0]);

    return (version == ENCRYPTION_VERSION_CURRENT);
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

QString SecureStringUtils::encryptStringForProfile(const QString& plaintext, const QString& profileName)
{
    if (plaintext.isEmpty() || profileName.isEmpty()) {
        return QString();
    }

    // Convert to UTF-8 bytes
    QByteArray plaintextBytes = plaintext.toUtf8();

    // Generate random salt
    QByteArray salt = generateSalt();

    // Get profile-specific encryption key
    QByteArray profileKey = getProfileEncryptionKey(profileName);

    if (profileKey.isEmpty()) {
        return QString();
    }

    // Derive encryption key using PBKDF2
    QByteArray derivedKey = generateKey(profileKey, salt, PBKDF2_ITERATIONS);

    if (derivedKey.isEmpty()) {
        return QString();
    }

    // Use Qt crypto encryption
    QByteArray nonce = generateNonce();
    QByteArray hmac;
    QByteArray encryptedData = encryptData(plaintextBytes, derivedKey, salt, nonce, hmac);

    if (encryptedData.isEmpty()) {
        // Securely clear sensitive data before returning
        secureByteArrayClear(plaintextBytes);
        secureByteArrayClear(derivedKey);
        secureByteArrayClear(profileKey);
        return QString();
    }

    // Build encrypted format: [VERSION:2][SALT:16][NONCE:16][HMAC:32][ENCRYPTED_DATA]
    QByteArray result;

    result.append(static_cast<char>(ENCRYPTION_VERSION_CURRENT));
    result.append(salt);
    result.append(nonce);
    result.append(hmac);
    result.append(encryptedData);

    // Securely clear sensitive data
    secureByteArrayClear(plaintextBytes);
    secureByteArrayClear(derivedKey);
    secureByteArrayClear(profileKey);

    // Encode as Base64 for safe text storage
    QString base64Result = result.toBase64();

    // Clear result data
    secureByteArrayClear(result);

    return base64Result;
}

QString SecureStringUtils::decryptStringForProfile(const QString& ciphertext, const QString& profileName)
{
    if (ciphertext.isEmpty() || profileName.isEmpty()) {
        return QString();
    }

    // Decode from Base64
    QByteArray encrypted = QByteArray::fromBase64(ciphertext.toLatin1());

    if (encrypted.size() < MIN_ENCRYPTED_SIZE) {
        return QString(); // Invalid format
    }

    // Extract version
    quint8 version = static_cast<quint8>(encrypted[0]);

    if (version != ENCRYPTION_VERSION_CURRENT) {
        return QString(); // Unsupported version
    }

    // Extract salt (bytes 1-16)
    QByteArray salt = encrypted.mid(1, SALT_SIZE);

    // Get profile-specific encryption key
    QByteArray profileKey = getProfileEncryptionKey(profileName);

    if (profileKey.isEmpty()) {
        return QString();
    }

    // Derive encryption key using PBKDF2
    QByteArray derivedKey = generateKey(profileKey, salt, PBKDF2_ITERATIONS);

    if (derivedKey.isEmpty()) {
        return QString();
    }

    // Current format: [VERSION:2][SALT:16][NONCE:16][HMAC:32][ENCRYPTED_DATA]
    if (encrypted.size() < 1 + SALT_SIZE + NONCE_SIZE + HMAC_SIZE) {
        return QString(); // Invalid format
    }

    // Extract nonce (bytes 17-32)
    QByteArray nonce = encrypted.mid(1 + SALT_SIZE, NONCE_SIZE);

    // Extract HMAC (bytes 33-64)
    QByteArray hmac = encrypted.mid(1 + SALT_SIZE + NONCE_SIZE, HMAC_SIZE);

    // Extract encrypted data (bytes 65+)
    QByteArray encryptedData = encrypted.mid(1 + SALT_SIZE + NONCE_SIZE + HMAC_SIZE);

    // Decrypt the data
    QByteArray decrypted = decryptData(encryptedData, derivedKey, salt, nonce, hmac);

    if (decrypted.isEmpty()) {
        // Securely clear sensitive data before returning
        secureByteArrayClear(derivedKey);
        secureByteArrayClear(profileKey);
        return QString();
    }

    // Convert back to QString
    QString result = QString::fromUtf8(decrypted);

    // Clear sensitive data
    secureByteArrayClear(encrypted);
    secureByteArrayClear(salt);
    secureByteArrayClear(profileKey);
    secureByteArrayClear(derivedKey);
    secureByteArrayClear(decrypted);

    return result;
}

QByteArray SecureStringUtils::getProfileEncryptionKey(const QString& profileName)
{
    // Try to load existing key from profile directory
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

    // Store the new key in profile directory
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
    return qEnvironmentVariableIsSet("MUDLET_TEST_MODE");
}

QByteArray SecureStringUtils::generateNonce()
{
    QByteArray nonce(NONCE_SIZE, 0);

    // Fill with cryptographically secure random bytes
    QRandomGenerator* rng = QRandomGenerator::system();

    for (int i = 0; i < NONCE_SIZE; ++i) {
        nonce[i] = static_cast<char>(rng->bounded(256));
    }

    return nonce;
}

QByteArray SecureStringUtils::encryptData(const QByteArray& plaintext, const QByteArray& key,
                                               const QByteArray& salt, const QByteArray& nonce,
                                               QByteArray& hmac)
{
    if (plaintext.isEmpty() || key.size() != KEY_SIZE || salt.size() != SALT_SIZE || nonce.size() != NONCE_SIZE) {
        return QByteArray();
    }

    // Create cipher key by combining derived key with nonce
    QByteArray cipherKey = QCryptographicHash::hash(key + nonce, QCryptographicHash::Sha256);

    // XOR encryption (simple but authenticated via HMAC)
    QByteArray encrypted = plaintext;

    for (int i = 0; i < encrypted.size(); ++i) {
        encrypted[i] = encrypted[i] ^ cipherKey[i % cipherKey.size()];
    }

    // Create HMAC-SHA256 for authentication
    // HMAC covers: salt + nonce + encrypted_data
    QByteArray macData = salt + nonce + encrypted;
    hmac = QMessageAuthenticationCode::hash(macData, key, QCryptographicHash::Sha256);

    // Clear sensitive data
    secureByteArrayClear(cipherKey);
    secureByteArrayClear(macData);

    return encrypted;
}

QByteArray SecureStringUtils::decryptData(const QByteArray& ciphertext, const QByteArray& key,
                                               const QByteArray& salt, const QByteArray& nonce,
                                               const QByteArray& hmac)
{
    if (ciphertext.isEmpty() || key.size() != KEY_SIZE || salt.size() != SALT_SIZE ||
        nonce.size() != NONCE_SIZE || hmac.size() != HMAC_SIZE) {
        return QByteArray();
    }

    // Verify HMAC first (authenticate before decrypt)
    QByteArray macData = salt + nonce + ciphertext;
    QByteArray expectedHmac = QMessageAuthenticationCode::hash(macData, key, QCryptographicHash::Sha256);

    // Constant-time comparison to prevent timing attacks
    bool hmacValid = (hmac.size() == expectedHmac.size());

    for (int i = 0; i < qMin(hmac.size(), expectedHmac.size()); ++i) {
        hmacValid &= (hmac[i] == expectedHmac[i]);
    }

    if (!hmacValid) {
        secureByteArrayClear(macData);
        secureByteArrayClear(expectedHmac);
        return QByteArray(); // Authentication failed
    }

    // Create cipher key by combining derived key with nonce
    QByteArray cipherKey = QCryptographicHash::hash(key + nonce, QCryptographicHash::Sha256);

    // XOR decryption (same operation as encryption)
    QByteArray decrypted = ciphertext;

    for (int i = 0; i < decrypted.size(); ++i) {
        decrypted[i] = decrypted[i] ^ cipherKey[i % cipherKey.size()];
    }

    // Clear sensitive data
    secureByteArrayClear(cipherKey);
    secureByteArrayClear(macData);
    secureByteArrayClear(expectedHmac);

    return decrypted;
}

// Convenience methods for password storage and retrieval

bool SecureStringUtils::storePassword(const QString& profileName, const QString& key, const QString& password)
{
    if (profileName.isEmpty() || key.isEmpty() || !isValidPasswordKey(key)) {
        return false;
    }

    if (password.isEmpty()) {
        // Allow storing empty passwords (effectively removing them)
        return removePassword(profileName, key);
    }

    QString filePath = getPasswordFilePath(profileName, key);

    // Ensure directory exists
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists() && !dir.mkpath(dir.absolutePath())) {
        qDebug() << "SecureStringUtils::storePassword() - Failed to create directory:" << dir.absolutePath();
        return false;
    }

    // Encrypt the password
    QString encryptedPassword = encryptStringForProfile(password, profileName);
    if (encryptedPassword.isEmpty()) {
        qDebug() << "SecureStringUtils::storePassword() - Failed to encrypt password";
        return false;
    }

    // Save to file
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        qDebug() << "SecureStringUtils::storePassword() - Failed to open file for writing:" << filePath << file.errorString();
        return false;
    }

    QDataStream ofs(&file);
    ofs.setVersion(QDataStream::Qt_5_12);
    ofs << encryptedPassword;

    if (!file.commit()) {
        qDebug() << "SecureStringUtils::storePassword() - Failed to commit file:" << filePath << file.errorString();
        return false;
    }

    return true;
}

QString SecureStringUtils::retrievePassword(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty() || !isValidPasswordKey(key)) {
        return QString();
    }

    QString filePath = getPasswordFilePath(profileName, key);

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        // File doesn't exist or can't be read - not an error, just no password stored
        return QString();
    }

    QDataStream ifs(&file);
    ifs.setVersion(QDataStream::Qt_5_12);

    QString encryptedPassword;
    ifs >> encryptedPassword;
    file.close();

    if (encryptedPassword.isEmpty()) {
        return QString();
    }

    // Decrypt the password
    return decryptStringForProfile(encryptedPassword, profileName);
}

bool SecureStringUtils::removePassword(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty() || !isValidPasswordKey(key)) {
        return false;
    }

    QString filePath = getPasswordFilePath(profileName, key);

    QFile file(filePath);
    if (!file.exists()) {
        // File doesn't exist - consider it successfully removed
        return true;
    }

    return file.remove();
}

bool SecureStringUtils::hasPassword(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty() || !isValidPasswordKey(key)) {
        return false;
    }

    QString filePath = getPasswordFilePath(profileName, key);
    return QFile::exists(filePath);
}

QString SecureStringUtils::getPasswordFilePath(const QString& profileName, const QString& key)
{
    // Use the same profile path structure as mudlet
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QString("%1/profiles/%2/passwords/%3.dat").arg(configPath, profileName, key);
}

bool SecureStringUtils::isValidPasswordKey(const QString& key)
{
    if (key.isEmpty() || key.length() > 100) {
        return false;
    }

    // Allow alphanumeric characters, underscores, and hyphens
    // This ensures the key is safe for use as a filename
    QRegularExpression validKeyRegex(qsl("^[a-zA-Z0-9_-]+$"));
    return validKeyRegex.match(key).hasMatch();
}
