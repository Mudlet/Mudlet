/**************************************************************************
**
**  macdocktile.h
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qtdocktile.h"
#include "qtdocktile_p.h"

#import <AppKit/NSDockTile.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSImageView.h>
#import <AppKit/NSCIImageRep.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSColor.h>
#import <Foundation/NSString.h>

@interface ApplicationProgressView : NSView {
    int min;
    int max;
    int value;
}

+ (ApplicationProgressView *)sharedProgressView;

- (void)setRangeMin:(int)v1 max:(int)v2;
- (void)setValue:(int)v;
- (void)updateBadge;

@end

static ApplicationProgressView *sharedProgressView = nil;

@implementation ApplicationProgressView

+ (ApplicationProgressView *)sharedProgressView
{
    if (sharedProgressView == nil) {
        sharedProgressView = [[ApplicationProgressView alloc] init];
    }
    return sharedProgressView;
}

- (void)setRangeMin:(int)v1 max:(int)v2
{
    min = v1;
    max = v2;
    [self updateBadge];
}

- (void)setValue:(int)v
{
    value = v;
    [self updateBadge];
}

- (void)updateBadge
{
    [[NSApp dockTile] display];
}

- (void)drawRect:(NSRect)rect
{
    Q_UNUSED(rect)
    NSRect boundary = [self bounds];
    [[NSApp applicationIconImage] drawInRect:boundary
        fromRect:NSZeroRect
        operation:NSCompositeCopy
        fraction:1.0];
    NSRect progressBoundary = boundary;
    progressBoundary.size.height *= 0.13;
    progressBoundary.size.width *= 0.8;
    progressBoundary.origin.x = (NSWidth(boundary) - NSWidth(progressBoundary))/2.;
    progressBoundary.origin.y = NSHeight(boundary)*0.13;

    double range = max - min;
    double percent = 0.50;
    if (range != 0)
        percent = (value - min) / range;
    if (percent > 1)
        percent = 1;
    else if (percent < 0)
        percent = 0;

    NSRect currentProgress = progressBoundary;
    currentProgress.size.width *= percent;
    [[NSColor blackColor] setFill];
    [NSBezierPath fillRect:progressBoundary];
    [[NSColor lightGrayColor] setFill];
    [NSBezierPath fillRect:currentProgress];
    [[NSColor blackColor] setStroke];
    [NSBezierPath strokeRect:progressBoundary];
}

@end

bool QtDockTile::isAvailable()
{
    return true;
}

void QtDockTilePrivate::setBadge(int badge)
{
    if (badge > 0)
        [[NSApp dockTile] setBadgeLabel:[[NSNumber numberWithInt:badge] stringValue]];
    else
        [[NSApp dockTile] setBadgeLabel:nil];
}

void QtDockTilePrivate::setProgress(int progress)
{
    [[ApplicationProgressView sharedProgressView] setRangeMin:0 max:100];
    [[ApplicationProgressView sharedProgressView] setValue:progress];
    if (progress) {
        [[NSApp dockTile] setContentView:[ApplicationProgressView sharedProgressView]];
    } else {
        [[NSApp dockTile] setContentView:nil];
    }
    [[NSApp dockTile] display];
}
