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

#include <QString>
#include <QByteArray>

/**
 * @brief Utility class for secure string operations
 *
 * This class provides cryptographically secure encryption for sensitive data like passwords
 * stored in configuration files. All encryption is profile-aware, using unique encryption
 * keys stored in profile directories.
 *
 * Features:
 * - Per-profile encryption keys stored in profile directories
 * - Qt-based encryption with PBKDF2-SHA256 key derivation and HMAC authentication
 * - Authenticated encryption with integrity verification
 * - Secure memory clearing
 * - Automatic migration from plaintext passwords
 * - Graceful degradation when SSL/TLS is unavailable
 *
 * Encrypted format: [VERSION:2][SALT:16][NONCE:16][HMAC:32][ENCRYPTED_DATA]
 * All encoded as Base64 for safe text storage in encrypted files.
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
     * @brief Check SSL backend configuration and report potential issues
     * @return QString with diagnostic information about SSL backend status
     */
    static QString getSSLBackendInfo();

    /**
     * @brief Check if running in test environment (to disable certain features during testing)
     * @return true if in test environment, false otherwise
     */
    static bool isTestEnvironment();

    // Convenience methods for password storage and retrieval

    /**
     * @brief Store an encrypted password for a profile and key
     * @param profileName Name of the profile
     * @param key Password identifier (e.g., "server_password", "proxy_password")
     * @param password Plaintext password to encrypt and store
     * @return true if stored successfully, false otherwise
     */
    static bool storePassword(const QString& profileName, const QString& key, const QString& password);

    /**
     * @brief Retrieve and decrypt a password for a profile and key
     * @param profileName Name of the profile
     * @param key Password identifier
     * @return Decrypted password, or empty string if not found or decryption failed
     */
    static QString retrievePassword(const QString& profileName, const QString& key);

    /**
     * @brief Remove a stored password for a profile and key
     * @param profileName Name of the profile
     * @param key Password identifier
     * @return true if removed successfully, false otherwise
     */
    static bool removePassword(const QString& profileName, const QString& key);

    /**
     * @brief Check if a password is stored for a profile and key
     * @param profileName Name of the profile
     * @param key Password identifier
     * @return true if password exists, false otherwise
     */
    static bool hasPassword(const QString& profileName, const QString& key);

private:
    // Password file storage helpers

    /**
     * @brief Generate the file path for storing a password
     * @param profileName Name of the profile
     * @param key Password identifier
     * @return Full path to password file
     */
    static QString getPasswordFilePath(const QString& profileName, const QString& key);

    /**
     * @brief Validate that a key name is safe for file storage
     * @param key Password identifier to validate
     * @return true if key is valid for file storage
     */
    static bool isValidPasswordKey(const QString& key);

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
     * @brief Load encryption key from profile directory file
     * @param profileName Name of the profile
     * @return 32-byte encryption key, or empty if not found/invalid
     */
    static QByteArray loadEncryptionKeyFromFile(const QString& profileName);

    /**
     * @brief Store encryption key to profile directory file
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
     * @brief Generate a random nonce for encryption
     * @return 16-byte random nonce
     */
    static QByteArray generateNonce();

    /**
     * @brief Encrypt data using XOR cipher + HMAC-SHA256
     * @param plaintext Data to encrypt
     * @param key 32-byte encryption key
     * @param salt 16-byte salt
     * @param nonce 16-byte nonce
     * @param hmac Output parameter for 32-byte HMAC
     * @return Encrypted data, or empty on failure
     */
    static QByteArray encryptData(const QByteArray& plaintext, const QByteArray& key,
                                 const QByteArray& salt, const QByteArray& nonce,
                                 QByteArray& hmac);

    /**
     * @brief Decrypt data using XOR cipher + HMAC-SHA256
     * @param ciphertext Encrypted data
     * @param key 32-byte encryption key
     * @param salt 16-byte salt
     * @param nonce 16-byte nonce
     * @param hmac 32-byte HMAC for verification
     * @return Decrypted data, or empty on failure/authentication error
     */
    static QByteArray decryptData(const QByteArray& ciphertext, const QByteArray& key,
                                 const QByteArray& salt, const QByteArray& nonce,
                                 const QByteArray& hmac);

    // Constants for the encrypted format
    static constexpr quint8 ENCRYPTION_VERSION_CURRENT = 2; // Current version
    static constexpr int SALT_SIZE = 16;
    static constexpr int NONCE_SIZE = 16; // Nonce size
    static constexpr int HMAC_SIZE = 32;  // HMAC-SHA256 size
    static constexpr int KEY_SIZE = 32;   // 256-bit key
    static constexpr int PBKDF2_ITERATIONS = 100000; // Strong key derivation
    static constexpr int MIN_ENCRYPTED_SIZE = 1 + SALT_SIZE + NONCE_SIZE + HMAC_SIZE; // version + salt + nonce + hmac + at least some data
};

#endif // SECURESTRINGUTILS_H
