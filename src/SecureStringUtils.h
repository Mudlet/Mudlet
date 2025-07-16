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

#ifndef SECURESTRINGUTILS_H
#define SECURESTRINGUTILS_H

#include "pre_guard.h"
#include <QString>
#include <QByteArray>
#include "post_guard.h"

/**
 * @brief Utility class for secure string operations
 * 
 * This class provides cryptographic encryption for sensitive data like passwords
 * stored in configuration files. It uses Qt's built-in cryptographic functions
 * for secure encryption and provides additional security features like:
 * 
 * - ChaCha20-like stream cipher encryption with random nonce per encryption
 * - PBKDF2 key derivation with salt using QCryptographicHash
 * - Secure memory clearing
 * - Backward compatibility detection for migration from plaintext
 * 
 * The encrypted format includes: [VERSION:1][SALT:16][NONCE:16][ENCRYPTED_DATA]
 * All encoded as Base64 for safe XML storage.
 */
class SecureStringUtils
{
public:
    /**
     * @brief Encrypt a plaintext string using AES-256-CTR
     * @param plaintext The string to encrypt
     * @return Base64-encoded encrypted string, or empty string if input is empty
     */
    static QString encryptString(const QString& plaintext);
    
    /**
     * @brief Decrypt an encrypted string
     * @param ciphertext Base64-encoded encrypted string
     * @return Decrypted plaintext, or empty string if input is empty/invalid
     */
    static QString decryptString(const QString& ciphertext);
    
    /**
     * @brief Safely encrypt a string, detecting if it's already encrypted
     * @param text String that may be plaintext or already encrypted
     * @return Encrypted string (unchanged if already encrypted)
     */
    static QString safeEncryptString(const QString& text);
    
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
     * @brief Get the base password for key derivation
     * @return Application-specific password
     */
    static QByteArray getBasePassword();
    
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
     * @brief Generate a random salt
     * @return 16-byte random salt
     */
    static QByteArray generateSalt();
    
    /**
     * @brief Generate a random nonce
     * @return 16-byte random nonce
     */
    static QByteArray generateNonce();
    
    /**
     * @brief Generate a cryptographic keystream
     * @param key 32-byte encryption key
     * @param nonce 16-byte nonce
     * @param length Length of keystream to generate
     * @return Generated keystream
     */
    static QByteArray generateKeystream(const QByteArray& key, const QByteArray& nonce, int length);
    
    // Constants for the encrypted format
    static constexpr quint8 ENCRYPTION_VERSION = 1;
    static constexpr int SALT_SIZE = 16;
    static constexpr int NONCE_SIZE = 16;
    static constexpr int KEY_SIZE = 32; // 256-bit key
    static constexpr int MIN_ENCRYPTED_SIZE = 1 + SALT_SIZE + NONCE_SIZE; // version + salt + nonce + at least some data
};

#endif // SECURESTRINGUTILS_H
