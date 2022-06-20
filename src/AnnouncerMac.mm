#include "Announcer.h"

#include <AppKit/AppKit.h>
#include <QDebug>

Announcer::Announcer(QObject *parent)
    : QObject{parent}
{

}

void Announcer::announce(QString text)
{
    NSDictionary *announcementInfo = @{
        NSAccessibilityAnnouncementKey : text.toNSString(),
        NSAccessibilityPriorityKey : @(NSAccessibilityPriorityHigh),
    };
    // AXLiveRegionChanged is also an option to look into, should this one not be good enough
    NSAccessibilityPostNotificationWithUserInfo([NSApp mainWindow], NSAccessibilityAnnouncementRequestedNotification, announcementInfo);
}
