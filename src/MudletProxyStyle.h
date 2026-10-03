/***************************************************************************
 *   Copyright (C) 2020 by Piotr Wilczynski - delwing@gmail.com            *
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

#ifndef MUDLET_MUDLETPROXYSTYLE_H
#define MUDLET_MUDLETPROXYSTYLE_H

#include <QAbstractNativeEventFilter>
#include <QProxyStyle>

#include "enums.h"

// Applies Mudlet's application-wide style hint adjustments on top of whichever
// base style is in use.
//
// Whether a lone Alt tap moves focus to the menu bar is the player's choice:
// always, never, or (the default) only when the operating system reports a
// screen reader. That report is SPI_GETSCREENREADER on Windows and
// org.a11y.Status.ScreenReaderEnabled on Linux and the BSDs, and not every
// screen reader makes it: Narrator does not set the Windows flag, and Orca
// started by hand, or on a desktop that does not set ScreenReaderEnabled,
// leaves it false. "Always" is for those players. QAccessible::isActive() is
// not used because any accessibility client turns it on, such as an IME or
// password manager on Windows, or the AT-SPI bus that most X11 sessions start
// without a screen reader. macOS has a native menu bar, so nothing is detected
// there.
//
// The choice is static because the application style is replaced on every
// appearance change, and DarkTheme wraps an instance of this class.
class MudletProxyStyle : public QProxyStyle, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    MudletProxyStyle();
    explicit MudletProxyStyle(const QString& style);
    static enums::MenuBarAltKeyNavigation altKeyNavigation() { return smAltKeyNavigation; }
    static void setAltKeyNavigation(const enums::MenuBarAltKeyNavigation mode) { smAltKeyNavigation = mode; }
    int styleHint(StyleHint styleHint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const override;
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
private slots:
    void slot_a11yStatusChanged(const QString& interfaceName, const QVariantMap& changedProperties, const QStringList& invalidatedProperties);
#endif

private:
    void watchScreenReaderStatus();

    inline static enums::MenuBarAltKeyNavigation smAltKeyNavigation = enums::MenuBarAltKeyNavigation::WhenScreenReaderRunning;
    // QMenuBar asks for SH_MenuBar_AltKeyNavigation on every event its window
    // receives, so the operating system is asked once and then told of changes
    bool mScreenReaderRunning = false;
};

#endif
