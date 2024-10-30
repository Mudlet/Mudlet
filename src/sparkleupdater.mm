/***************************************************************************
 *   Copyright (C) 2024 by Mudlet makers                                   *
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

#include "sparkleupdater.h"
#include <qaction.h>

#import <Sparkle/Sparkle.h>

@interface SparkleUpdaterDelegate : NSObject <SPUUpdaterDelegate>

@property(nonatomic, strong) SPUStandardUpdaterController* updaterController;

@end

@implementation SparkleUpdaterDelegate

- (void)updater:(SPUUpdater*)updater didFailToStartWithError:(NSError*)error
{
    NSLog(@"Sparkle updater failed to start with error: %@", error);
}

@end

SparkleUpdater::SparkleUpdater()
{
    @autoreleasepool {
        _updaterDelegate = [[SparkleUpdaterDelegate alloc] init];
        _updaterDelegate.updaterController = [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES updaterDelegate:_updaterDelegate userDriverDelegate:nil];
        // necessary because we used -setFeedURL: in the past and now Sparkle wants us to migrate to setting the feed url in Info.plist
        [_updaterDelegate.updaterController.updater clearFeedURLFromUserDefaults];
    }
}

SparkleUpdater::~SparkleUpdater() {}

// Called when the user checks for updates
void SparkleUpdater::checkForUpdates()
{
    @autoreleasepool {
        [_updaterDelegate.updaterController checkForUpdates:nil];
    }
}

void SparkleUpdater::setAutomaticallyChecksForUpdates(bool on)
{
    @autoreleasepool {
        [_updaterDelegate.updaterController.updater setAutomaticallyChecksForUpdates:on];
    }
}
bool SparkleUpdater::automaticallyChecksForUpdates()
{
    @autoreleasepool {
        return [_updaterDelegate.updaterController.updater automaticallyChecksForUpdates];
    }
}

void SparkleUpdater::setAutomaticallyDownloadsUpdates(bool on)
{
    @autoreleasepool {
        [_updaterDelegate.updaterController.updater setAutomaticallyDownloadsUpdates:on];
    }
}

bool SparkleUpdater::automaticallyDownloadsUpdates()
{
    @autoreleasepool {
        return [_updaterDelegate.updaterController.updater automaticallyDownloadsUpdates];
    }
}