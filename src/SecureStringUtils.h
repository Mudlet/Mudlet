#ifndef SECURESTRINGUTILS_H
#define SECURESTRINGUTILS_H

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

#include "pre_guard.h"
#include <QString>
#include <QByteArray>
#include "post_guard.h"

/**
 * @brief Utility class for secure string operations
 * 
 * This class provides cryptographically secure encryption for sensitive data like passwords
 * stored in configuration files. All encryption is profile-aware, using unique encryption
 * keys stored in platform secure storage (QtKeychain).
 * 
 * Features:
 * - Per-profile encryption keys stored in secure platform storage
 * - AES-256-GCM encryption with PBKDF2-SHA256 key derivation (when OpenSSL available)
 * - Fallback to Qt-based encryption for builds without OpenSSL support
 * - Authenticated encryption with integrity verification
 * - Secure memory clearing
 * - Automatic migration from plaintext passwords
 * - Graceful degradation when SSL/TLS is unavailable
 * 
 * Encrypted format (OpenSSL): [VERSION:1][SALT:16][IV:12][TAG:16][ENCRYPTED_DATA]
 * Encrypted format (Qt fallback): [VERSION:2][SALT:16][NONCE:16][HMAC:32][ENCRYPTED_DATA]
 * All encoded as Base64 for safe text storage (QtKeychain, encrypted files).
 */
class SecureStringUtils
{
public:
    /**
     * @brief Encrypt a string using a profile-specific encryption key
     * @param plaintext The string to encrypt
     * @param profileName Name of the profile (used for key lookup)
     * @return Base64-encoded encrypted string, or empty string if input is empty
     */
    static QString encryptStringForProfile(const QString& plaintext, const QString& profileName);
    
    /**
     * @brief Decrypt a string using a profile-specific encryption key
     * @param ciphertext Base64-encoded encrypted string
     * @param profileName Name of the profile (used for key lookup)
     * @return Decrypted plaintext, or empty string if input is empty/invalid
     */
    static QString decryptStringForProfile(const QString& ciphertext, const QString& profileName);
    
    /**
     * @brief Check if a string appears to be in encrypted format
     * @param text String to check
     * @return true if the string appears to be encrypted
     */
    static bool isEncryptedFormat(const QString& text);
    
    /**
     * @brief Securely clear a QString from memory
     * @param str String to clear
     */
    static void secureStringClear(QString& str);
    
    /**
     * @brief Securely clear a QByteArray from memory
     * @param array Array to clear
     */
    static void secureByteArrayClear(QByteArray& array);
    
    /**
     * @brief Check if OpenSSL crypto is available for secure encryption
     * @return true if OpenSSL is available, false if fallback required
     */
    static bool isOpenSSLAvailable();

    /**
     * @brief Check if running in test environment (to avoid keychain prompts)
     * @return true if in test environment, false otherwise
     */
    static bool isTestEnvironment();

private:
    /**
     * @brief Generate a cryptographic key using PBKDF2
     * @param password Base password/passphrase
     * @param salt Salt for key derivation
     * @param iterations Number of PBKDF2 iterations
     * @return 32-byte key
     */
    static QByteArray generateKey(const QByteArray& password, const QByteArray& salt, int iterations = 10000);
    
    /**
     * @brief Get or create a profile-specific encryption key
     * @param profileName Name of the profile
     * @return 32-byte encryption key for the profile
     */
    static QByteArray getProfileEncryptionKey(const QString& profileName);
    
    /**
     * @brief Store a profile-specific encryption key in secure storage
     * @param profileName Name of the profile
     * @param key 32-byte encryption key to store
     * @return true if storage was successful
     */
    static bool storeProfileEncryptionKey(const QString& profileName, const QByteArray& key);
    
    /**
     * @brief Load encryption key from profile directory file (portable mode)
     * @param profileName Name of the profile
     * @return 32-byte encryption key, or empty if not found/invalid
     */
    static QByteArray loadEncryptionKeyFromFile(const QString& profileName);
    
    /**
     * @brief Store encryption key to profile directory file (portable mode)
     * @param profileName Name of the profile
     * @param key 32-byte encryption key to store
     * @return true if storage was successful
     */
    static bool storeEncryptionKeyToFile(const QString& profileName, const QByteArray& key);
    
    /**
     * @brief Generate a random salt
     * @return 16-byte random salt
     */
    static QByteArray generateSalt();
    
    /**
     * @brief Generate a random IV for AES-GCM
     * @return 12-byte random IV
     */
    static QByteArray generateIV();
    
    /**
     * @brief Encrypt data using AES-256-GCM
     * @param plaintext Data to encrypt
     * @param key 32-byte AES key
     * @param iv 12-byte initialization vector
     * @param tag Output parameter for 16-byte authentication tag
     * @return Encrypted data, or empty on failure
     */
    static QByteArray encryptAES(const QByteArray& plaintext, const QByteArray& key, 
                                const QByteArray& iv, QByteArray& tag);
    
    /**
     * @brief Decrypt data using AES-256-GCM
     * @param ciphertext Encrypted data
     * @param key 32-byte AES key
     * @param iv 12-byte initialization vector
     * @param tag 16-byte authentication tag
     * @return Decrypted data, or empty on failure/authentication error
     */
    static QByteArray decryptAES(const QByteArray& ciphertext, const QByteArray& key,
                                const QByteArray& iv, const QByteArray& tag);
    
    /**
     * @brief Generate a random nonce for Qt fallback encryption
     * @return 16-byte random nonce
     */
    static QByteArray generateNonce();
    
    /**
     * @brief Encrypt data using Qt fallback (XOR cipher + HMAC-SHA256)
     * @param plaintext Data to encrypt
     * @param key 32-byte encryption key
     * @param salt 16-byte salt
     * @param nonce 16-byte nonce
     * @param hmac Output parameter for 32-byte HMAC
     * @return Encrypted data, or empty on failure
     */
    static QByteArray encryptQtFallback(const QByteArray& plaintext, const QByteArray& key,
                                       const QByteArray& salt, const QByteArray& nonce,
                                       QByteArray& hmac);
    
    /**
     * @brief Decrypt data using Qt fallback (XOR cipher + HMAC-SHA256)
     * @param ciphertext Encrypted data
     * @param key 32-byte encryption key
     * @param salt 16-byte salt
     * @param nonce 16-byte nonce
     * @param hmac 32-byte HMAC for verification
     * @return Decrypted data, or empty on failure/authentication error
     */
    static QByteArray decryptQtFallback(const QByteArray& ciphertext, const QByteArray& key,
                                       const QByteArray& salt, const QByteArray& nonce,
                                       const QByteArray& hmac);
    
    // Constants for the encrypted format
    static constexpr quint8 ENCRYPTION_VERSION_OPENSSL = 1;  // OpenSSL AES-GCM
    static constexpr quint8 ENCRYPTION_VERSION_QT_FALLBACK = 2; // Qt crypto fallback
    static constexpr int SALT_SIZE = 16;
    static constexpr int IV_SIZE = 12;    // AES-GCM standard IV size  
    static constexpr int NONCE_SIZE = 16; // Qt fallback nonce size
    static constexpr int TAG_SIZE = 16;   // AES-GCM authentication tag size
    static constexpr int HMAC_SIZE = 32;  // Qt fallback HMAC-SHA256 size
    static constexpr int KEY_SIZE = 32;   // 256-bit key
    static constexpr int PBKDF2_ITERATIONS = 100000; // Strong key derivation
    static constexpr int MIN_ENCRYPTED_SIZE = 1 + SALT_SIZE + IV_SIZE + TAG_SIZE; // version + salt + iv + tag + at least some data
};

#endif // SECURESTRINGUTILS_H
