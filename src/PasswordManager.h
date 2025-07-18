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

#ifndef MUDLET_PASSWORDMANAGER_H
#define MUDLET_PASSWORDMANAGER_H

#include "pre_guard.h"
#include <QString>
#include "post_guard.h"

class PasswordManager
{
public:
    // Store a password securely for a given profile and key
    // Uses QtKeychain when available, falls back to encrypted storage for portability
    static bool storePassword(const QString& profileName, const QString& key, const QString& password);
    
    // Retrieve a password securely for a given profile and key
    // Handles both QtKeychain and encrypted storage automatically
    static QString retrievePassword(const QString& profileName, const QString& key);
    
    // Remove a stored password
    static bool removePassword(const QString& profileName, const QString& key);
    
    // Check if QtKeychain is available and working
    static bool isKeychainAvailable();

private:
    // Try to store in QtKeychain first
    static bool storeInKeychain(const QString& profileName, const QString& key, const QString& password);
    
    // Try to retrieve from QtKeychain
    static QString retrieveFromKeychain(const QString& profileName, const QString& key);
    
    // Remove from QtKeychain
    static bool removeFromKeychain(const QString& profileName, const QString& key);
    
    // Fallback: store encrypted in profile directory (portable mode)
    static bool storeEncrypted(const QString& profileName, const QString& key, const QString& password);
    
    // Fallback: retrieve encrypted from profile directory
    static QString retrieveEncrypted(const QString& profileName, const QString& key);
    
    // Remove encrypted file
    static bool removeEncrypted(const QString& profileName, const QString& key);
    
    // Generate keychain service name
    static QString generateServiceName(const QString& profileName, const QString& key);
    
    // Generate file path for encrypted storage
    static QString generateFilePath(const QString& profileName, const QString& key);
};

#endif // MUDLET_PASSWORDMANAGER_H
