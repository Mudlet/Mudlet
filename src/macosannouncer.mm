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
    NSView* view = reinterpret_cast<NSView*>(mudlet::self()->effectiveWinId());
    if (!view) {
        qDebug() << "no view found";
    } else {
        qDebug() << view;
    }

    NSString* msg = @"Hello World!";
    NSDictionary *announcementInfo = @{
        NSAccessibilityAnnouncementKey : msg,
        NSAccessibilityPriorityKey : @(NSAccessibilityPriorityHigh),
    };
    NSAccessibilityPostNotificationWithUserInfo(view, NSAccessibilityAnnouncementRequestedNotification, announcementInfo);
}
