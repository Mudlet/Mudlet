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
#include <QRandomGenerator>
#include "post_guard.h"

QString SecureStringUtils::encryptString(const QString& plaintext)
{
    if (plaintext.isEmpty()) {
        return QString();
    }
    
    // Convert to UTF-8 bytes
    QByteArray plaintextBytes = plaintext.toUtf8();
    
    // Generate deterministic key
    QByteArray key = generateKey();
    
    // Encrypt using XOR
    QByteArray encrypted = xorEncryptDecrypt(plaintextBytes, key);
    
    // Encode as Base64 for safe storage in XML
    QString result = encrypted.toBase64();
    
    // Clear sensitive data
    secureByteArrayClear(plaintextBytes);
    secureByteArrayClear(key);
    secureByteArrayClear(encrypted);
    
    return result;
}

QString SecureStringUtils::decryptString(const QString& ciphertext)
{
    if (ciphertext.isEmpty()) {
        return QString();
    }
    
    // Decode from Base64
    QByteArray encrypted = QByteArray::fromBase64(ciphertext.toLatin1());
    if (encrypted.isEmpty()) {
        return QString(); // Invalid Base64 or empty
    }
    
    // Generate the same deterministic key
    QByteArray key = generateKey();
    
    // Decrypt using XOR
    QByteArray decrypted = xorEncryptDecrypt(encrypted, key);
    
    // Convert back to QString
    QString result = QString::fromUtf8(decrypted);
    
    // Clear sensitive data
    secureByteArrayClear(encrypted);
    secureByteArrayClear(key);
    secureByteArrayClear(decrypted);
    
    return result;
}

void SecureStringUtils::secureStringClear(QString& str)
{
    // Overwrite string data with zeros before clearing
    if (!str.isEmpty()) {
        str.fill('0');
        str.clear();
        str.squeeze(); // Free memory if possible
    }
}

void SecureStringUtils::secureByteArrayClear(QByteArray& array)
{
    // Overwrite array data with zeros before clearing
    if (!array.isEmpty()) {
        array.fill(0);
        array.clear();
        array.squeeze(); // Free memory if possible
    }
}

QByteArray SecureStringUtils::generateKey()
{
    // Generate a deterministic key based on application name and a fixed salt
    // This provides basic obfuscation for config files
    // NOTE: This is NOT cryptographically secure - it's just to prevent casual inspection
    const QString seed = QCoreApplication::applicationName() + qsl("MudletProxyPasswordSalt2025");
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(seed.toUtf8());
    QByteArray key = hash.result();
    
    return key;
}

QByteArray SecureStringUtils::xorEncryptDecrypt(const QByteArray& data, const QByteArray& key)
{
    if (data.isEmpty() || key.isEmpty()) {
        return QByteArray();
    }
    
    QByteArray result;
    result.reserve(data.size());
    
    for (int i = 0; i < data.size(); ++i) {
        result.append(data[i] ^ key[i % key.size()]);
    }
    
    return result;
}
