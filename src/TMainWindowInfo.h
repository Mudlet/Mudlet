#ifndef MUDLET_TMAINWINDOWINFO_H
#define MUDLET_TMAINWINDOWINFO_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QSizeF>

// What getMainWindowInfo() tells scripts of their profile's main window. Sizes are in logical pixels (Qt's
// device-independent pixels, Android's dp, CSS's px), which every size class system's breakpoints are given in;
// on the desktop they are Mudlet's own pixels.
struct TMainWindowInfo
{
    // Material Design's window size classes
    enum class SizeClass { compact, medium, expanded };
    enum class Pointer { fine, coarse };

    // What the frontend knows of where it draws the main window, which Mudlet cannot measure where it does not
    // draw it itself
    struct Device
    {
        Pointer pointer = Pointer::fine;
        bool hover = true;
        // Physical pixels per logical pixel
        double devicePixelRatio = 1.0;
        // Mudlet's pixels per logical pixel, as the frontend scales the main window
        double pixelScale = 1.0;

        bool operator==(const Device&) const = default;
    };

    // getMainWindowSize()'s size over device.pixelScale
    QSizeF size;
    Device device;

    static SizeClass widthClass(const double width) { return width < 600 ? SizeClass::compact : width < 840 ? SizeClass::medium : SizeClass::expanded; }
    static SizeClass heightClass(const double height) { return height < 480 ? SizeClass::compact : height < 900 ? SizeClass::medium : SizeClass::expanded; }
    static const char* className(const SizeClass sizeClass)
    {
        switch (sizeClass) {
        case SizeClass::compact:
            return "compact";
        case SizeClass::medium:
            return "medium";
        case SizeClass::expanded:
            return "expanded";
        }
        return "compact";
    }
    // As CSS's orientation media feature reads a viewport: a square one is portrait
    bool portrait() const { return size.height() >= size.width(); }

    // What sysMainWindowClassChangeEvent is raised for: everything but the exact size, which sysWindowResizeEvent tells
    bool sameClasses(const TMainWindowInfo& other) const
    {
        return widthClass(size.width()) == widthClass(other.size.width()) && heightClass(size.height()) == heightClass(other.size.height()) && portrait() == other.portrait() && device == other.device;
    }
};

#endif // MUDLET_TMAINWINDOWINFO_H
