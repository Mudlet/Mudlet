/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#ifndef MUDLET_MACMICROPHONEPERMISSION_H
#define MUDLET_MACMICROPHONEPERMISSION_H

#include <functional>

// Native macOS microphone permission handling using AVFoundation.
// This is needed because Qt's permission API requires proper app signing
// with entitlements, which development builds don't have.

class MacMicrophonePermission
{
public:
    enum class AuthorizationStatus {
        NotDetermined, // User has not yet been asked for permission
        Restricted,    // User cannot change this setting (parental controls, etc.)
        Denied,        // User explicitly denied permission
        Authorized     // User granted permission
    };

    // Prevent instantiation - this is a static-only utility class
    MacMicrophonePermission() = delete;
    MacMicrophonePermission(const MacMicrophonePermission&) = delete;
    MacMicrophonePermission& operator=(const MacMicrophonePermission&) = delete;

    // Check current authorization status without prompting the user
    static AuthorizationStatus checkStatus();

    // Request microphone access. The callback is called with true if granted, false otherwise.
    // Note: The callback is dispatched to the main thread by the implementation, so callers
    // can safely interact with Qt UI objects from within it.
    static void requestAccess(std::function<void(bool)> callback);
};

#endif // MUDLET_MACMICROPHONEPERMISSION_H
