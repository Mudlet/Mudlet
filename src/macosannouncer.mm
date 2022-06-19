#include "macosannouncer.h"

// appkit has to be included above mudlet.h since Boost::Collections is a clash
#include <AppKit/AppKit.h>
// also needed to make the two play nicely
#undef nil
#include "mudlet.h"
#define nil nullptr

#include <QDebug>
#include <QAccessible>
#include <QAccessibleEvent>

MacOSAnnouncer::MacOSAnnouncer(QObject *parent)
    : QObject{parent}
{

}

void MacOSAnnouncer::announce(QString text)
{
    NSDictionary *announcementInfo = @{
        NSAccessibilityAnnouncementKey : text.toNSString(),
        NSAccessibilityPriorityKey : @(NSAccessibilityPriorityHigh),
    };
    // AXLiveRegionChanged is also an option to look into, should this one not be good enough
    NSAccessibilityPostNotificationWithUserInfo([NSApp mainWindow], NSAccessibilityAnnouncementRequestedNotification, announcementInfo);
}
