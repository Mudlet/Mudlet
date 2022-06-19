#include "macosannouncer.h"

// appkit has to be included above mudlet.h since Boost::Collections is a clash
#include <AppKit/AppKit.h>
// also needed to make the two play nicely
#undef nil
#include "mudlet.h"
#define nil nullptr

#include <QDebug>

MacOSAnnouncer::MacOSAnnouncer(QObject *parent)
    : QObject{parent}
{

}

void MacOSAnnouncer::announce(QString text)
{
    auto element = mudlet::self()->effectiveWinId();
    qDebug() << "announcing"<<text;
    NSAccessibilityPostNotification(element, NSAccessibilityAnnouncementRequestedNotification);
}
