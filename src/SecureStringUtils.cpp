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
// OpenSSL includes for AES-GCM encryption
#ifdef MUDLET_USE_OPENSSL_CRYPTO
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#endif
// Qt includes for fallback encryption and SSL availability check
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif
#include "post_guard.h"

bool SecureStringUtils::isOpenSSLAvailable()
{
#ifdef MUDLET_USE_OPENSSL_CRYPTO
    // We have OpenSSL compiled in, so we can use its crypto functions
    // This is independent of Qt's SSL socket backend choice
    return true;
#else
    return false; // OpenSSL not compiled in
#endif
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
    
    try {
        // Try to decode and check structure
        QByteArray decoded = QByteArray::fromBase64(text.toLatin1());

        if (decoded.size() < MIN_ENCRYPTED_SIZE) {
            return false;
        }
        
        // Check version byte - support both OpenSSL and Qt fallback versions
        quint8 version = static_cast<quint8>(decoded[0]);
        return (version == ENCRYPTION_VERSION_OPENSSL || version == ENCRYPTION_VERSION_QT_FALLBACK);
        
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

QByteArray SecureStringUtils::generateIV()
{
    QByteArray iv;
    iv.resize(IV_SIZE);
    
#ifdef MUDLET_USE_OPENSSL_CRYPTO
    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), IV_SIZE) != 1) {
        // Fallback to Qt's random generator if OpenSSL fails
        QRandomGenerator* rng = QRandomGenerator::system();
        for (int i = 0; i < IV_SIZE; ++i) {
            iv[i] = static_cast<char>(rng->bounded(256));
        }
    }
#else
    // Use Qt's random generator when OpenSSL not available
    QRandomGenerator* rng = QRandomGenerator::system();
    for (int i = 0; i < IV_SIZE; ++i) {
        iv[i] = static_cast<char>(rng->bounded(256));
    }
#endif
    
    return iv;
}

QByteArray SecureStringUtils::encryptAES(const QByteArray& plaintext, const QByteArray& key, 
                                        const QByteArray& iv, QByteArray& tag)
{
#ifdef MUDLET_USE_OPENSSL_CRYPTO
    if (plaintext.isEmpty() || key.size() != KEY_SIZE || iv.size() != IV_SIZE) {
        return QByteArray();
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return QByteArray();
    }
    
    QByteArray ciphertext;
    int len;
    int ciphertext_len;
    
    try {
        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Set IV length
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Initialize key and IV
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, 
                              reinterpret_cast<const unsigned char*>(key.data()), 
                              reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Encrypt plaintext
        ciphertext.resize(plaintext.size() + 16); // Extra space for safety
        if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), 
                             &len, reinterpret_cast<const unsigned char*>(plaintext.data()), 
                             plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        ciphertext_len = len;
        
        // Finalize encryption
        if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        ciphertext_len += len;
        ciphertext.resize(ciphertext_len);
        
        // Get the authentication tag
        tag.resize(TAG_SIZE);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        EVP_CIPHER_CTX_free(ctx);
        return ciphertext;
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
#else
    Q_UNUSED(plaintext)
    Q_UNUSED(key)
    Q_UNUSED(iv)
    Q_UNUSED(tag)
    // OpenSSL not available - this should not be called in Qt fallback mode
    return QByteArray();
#endif
}

QByteArray SecureStringUtils::decryptAES(const QByteArray& ciphertext, const QByteArray& key,
                                        const QByteArray& iv, const QByteArray& tag)
{
#ifdef MUDLET_USE_OPENSSL_CRYPTO
    if (ciphertext.isEmpty() || key.size() != KEY_SIZE || iv.size() != IV_SIZE || tag.size() != TAG_SIZE) {
        return QByteArray();
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return QByteArray();
    }
    
    QByteArray plaintext;
    int len;
    int plaintext_len;
    
    try {
        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Set IV length
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Initialize key and IV
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, 
                              reinterpret_cast<const unsigned char*>(key.data()), 
                              reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Decrypt ciphertext
        plaintext.resize(ciphertext.size() + 16); // Extra space for safety
        if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), 
                             &len, reinterpret_cast<const unsigned char*>(ciphertext.data()), 
                             ciphertext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        plaintext_len = len;
        
        // Set expected tag value
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, 
                               const_cast<void*>(reinterpret_cast<const void*>(tag.data()))) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        
        // Finalize decryption (this will verify the tag)
        if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1) {
            // Authentication failed
            EVP_CIPHER_CTX_free(ctx);
            return QByteArray();
        }
        plaintext_len += len;
        plaintext.resize(plaintext_len);
        
        EVP_CIPHER_CTX_free(ctx);
        return plaintext;
        
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
#else
    Q_UNUSED(ciphertext)
    Q_UNUSED(key)
    Q_UNUSED(iv)
    Q_UNUSED(tag)
    // OpenSSL not available - this should not be called in Qt fallback mode
    return QByteArray();
#endif
}

QString SecureStringUtils::encryptStringForProfile(const QString& plaintext, const QString& profileName)
{
    if (plaintext.isEmpty() || profileName.isEmpty()) {
        return QString();
    }
    
    try {
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
        
        QByteArray result;
        
        // Choose encryption method based on OpenSSL availability
        if (isOpenSSLAvailable()) {
            // Use OpenSSL AES-256-GCM encryption
            QByteArray iv = generateIV();
            QByteArray tag;
            QByteArray encryptedData = encryptAES(plaintextBytes, derivedKey, iv, tag);

            if (encryptedData.isEmpty()) {
                // Securely clear sensitive data before returning
                secureByteArrayClear(plaintextBytes);
                secureByteArrayClear(derivedKey);
                secureByteArrayClear(profileKey);
                return QString();
            }
            
            // Build OpenSSL format: [VERSION:1][SALT:16][IV:12][TAG:16][ENCRYPTED_DATA]
            result.append(static_cast<char>(ENCRYPTION_VERSION_OPENSSL));
            result.append(salt);
            result.append(iv);
            result.append(tag);
            result.append(encryptedData);
        } else {
            // Use Qt fallback encryption
            QByteArray nonce = generateNonce();
            QByteArray hmac;
            QByteArray encryptedData = encryptQtFallback(plaintextBytes, derivedKey, salt, nonce, hmac);

            if (encryptedData.isEmpty()) {
                // Securely clear sensitive data before returning
                secureByteArrayClear(plaintextBytes);
                secureByteArrayClear(derivedKey);
                secureByteArrayClear(profileKey);
                return QString();
            }
            
            // Build Qt fallback format: [VERSION:2][SALT:16][NONCE:16][HMAC:32][ENCRYPTED_DATA]
            result.append(static_cast<char>(ENCRYPTION_VERSION_QT_FALLBACK));
            result.append(salt);
            result.append(nonce);
            result.append(hmac);
            result.append(encryptedData);
        }
        
        // Securely clear sensitive data
        secureByteArrayClear(plaintextBytes);
        secureByteArrayClear(derivedKey);
        secureByteArrayClear(profileKey);
        
        // Encode as Base64 for safe text storage
        QString base64Result = result.toBase64();
        
        // Clear result data
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

        if (version != ENCRYPTION_VERSION_OPENSSL && version != ENCRYPTION_VERSION_QT_FALLBACK) {
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
        
        QByteArray decrypted;
        
        if (version == ENCRYPTION_VERSION_OPENSSL) {
            // OpenSSL AES-GCM format: [VERSION:1][SALT:16][IV:12][TAG:16][ENCRYPTED_DATA]
            if (encrypted.size() < 1 + SALT_SIZE + IV_SIZE + TAG_SIZE) {
                return QString(); // Invalid format
            }
            
            // Extract IV (bytes 17-28)
            QByteArray iv = encrypted.mid(1 + SALT_SIZE, IV_SIZE);
            
            // Extract authentication tag (bytes 29-44)
            QByteArray tag = encrypted.mid(1 + SALT_SIZE + IV_SIZE, TAG_SIZE);
            
            // Extract encrypted data (bytes 45+)
            QByteArray encryptedData = encrypted.mid(1 + SALT_SIZE + IV_SIZE + TAG_SIZE);
            
            // Decrypt using AES-256-GCM
            decrypted = decryptAES(encryptedData, derivedKey, iv, tag);
        } else if (version == ENCRYPTION_VERSION_QT_FALLBACK) {
            // Qt fallback format: [VERSION:2][SALT:16][NONCE:16][HMAC:32][ENCRYPTED_DATA]
            if (encrypted.size() < 1 + SALT_SIZE + NONCE_SIZE + HMAC_SIZE) {
                return QString(); // Invalid format
            }
            
            // Extract nonce (bytes 17-32)
            QByteArray nonce = encrypted.mid(1 + SALT_SIZE, NONCE_SIZE);
            
            // Extract HMAC (bytes 33-64)
            QByteArray hmac = encrypted.mid(1 + SALT_SIZE + NONCE_SIZE, HMAC_SIZE);
            
            // Extract encrypted data (bytes 65+)
            QByteArray encryptedData = encrypted.mid(1 + SALT_SIZE + NONCE_SIZE + HMAC_SIZE);
            
            // Decrypt using Qt fallback
            decrypted = decryptQtFallback(encryptedData, derivedKey, salt, nonce, hmac);
        }
        
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
        // In test environment, skip keychain entirely
        if (isTestEnvironment()) {
            // Skip keychain operations in test mode
        } else {
            // Try to load existing key from secure storage first
            // Currently using file fallback for simplicity and reliability
            // Future improvement: integrate with async keychain API
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
    
    // Currently using file storage for encryption keys to maintain reliability
    // This provides a stable foundation while keychain integration can be added later
    return false; // Use file storage for encryption keys
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

QByteArray SecureStringUtils::encryptQtFallback(const QByteArray& plaintext, const QByteArray& key,
                                               const QByteArray& salt, const QByteArray& nonce,
                                               QByteArray& hmac)
{
    if (plaintext.isEmpty() || key.size() != KEY_SIZE || salt.size() != SALT_SIZE || nonce.size() != NONCE_SIZE) {
        return QByteArray();
    }
    
    try {
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
        
    } catch (...) {
        return QByteArray();
    }
}

QByteArray SecureStringUtils::decryptQtFallback(const QByteArray& ciphertext, const QByteArray& key,
                                               const QByteArray& salt, const QByteArray& nonce,
                                               const QByteArray& hmac)
{
    if (ciphertext.isEmpty() || key.size() != KEY_SIZE || salt.size() != SALT_SIZE || 
        nonce.size() != NONCE_SIZE || hmac.size() != HMAC_SIZE) {
        return QByteArray();
    }
    
    try {
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
        
    } catch (...) {
        return QByteArray();
    }
}
