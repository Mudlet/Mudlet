#ifndef MUDLET_SECURESTRINGUTILS_H
#define MUDLET_SECURESTRINGUTILS_H

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
#include <QByteArray>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QString>
#include "post_guard.h"

class SecureStringUtils
{
public:
    // Encrypt a string using XOR obfuscation with SHA-256 derived key (NOT for high-security use)
    // This is intended to prevent casual inspection of proxy passwords in config files
    static QString encryptString(const QString& plaintext);
    
    // Decrypt a string encrypted with encryptString
    static QString decryptString(const QString& ciphertext);
    
    // Secure memory clearing for QString
    static void secureStringClear(QString& str);
    
    // Secure memory clearing for QByteArray
    static void secureByteArrayClear(QByteArray& array);

private:
    // Generate a deterministic key from a fixed seed (for config file encryption)
    static QByteArray generateKey();
    
    // Simple XOR-based encryption with key
    static QByteArray xorEncryptDecrypt(const QByteArray& data, const QByteArray& key);
};

#endif // MUDLET_SECURESTRINGUTILS_H
