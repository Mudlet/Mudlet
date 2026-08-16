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

#include "MacMicrophonePermission.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

MacMicrophonePermission::AuthorizationStatus MacMicrophonePermission::checkStatus()
{
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    
    switch (status) {
        case AVAuthorizationStatusAuthorized:
            return AuthorizationStatus::Authorized;
        case AVAuthorizationStatusDenied:
            return AuthorizationStatus::Denied;
        case AVAuthorizationStatusRestricted:
            return AuthorizationStatus::Restricted;
        case AVAuthorizationStatusNotDetermined:
        default:
            return AuthorizationStatus::NotDetermined;
    }
}

void MacMicrophonePermission::requestAccess(std::function<void(bool)> callback)
{
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
        // The completion handler is called on an arbitrary queue,
        // so dispatch to main thread for UI-safe callback invocation.
        if (callback) {
            dispatch_async(dispatch_get_main_queue(), ^{
                callback(granted);
            });
        }
    }];
}
