#ifndef MUDLET_UTILS_H
#define MUDLET_UTILS_H

/***************************************************************************
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@hey.com            *
 *   Copyright (C) 2021, 2023, 2025 by Stephen Lyons                       *
 *                                               - slysven@virginmedia.com *
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

#include <QApplication>
#include <QEnterEvent>
#include <QDir>
#include <QRegularExpression>
#include <QString>
#include <QScreen>
#include <QWidget>

#define qsl(s) QStringLiteral(s)

using TEnterEvent = QEnterEvent;

class utils
{
public:
    // This construct will be very useful for formatting tooltips and by
    // defining a static function/method here we can save using the same
    // qsl all over the place:
    static QString richText(const QString& text) { return qsl("<p>%1</p>").arg(text); }

    // Qt 6.9 deprecated QDateTime::setOffsetFromUtc(int) and made it hard to
    // replicate the exact strings that we had before:
    static QString dateStamp() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        auto localNow = QDateTime::currentDateTime();
        const int offset = localNow.offsetFromUtc();
        if (offset) {
            unsigned hoursOff = abs(offset/3600);
            unsigned minutesOff = (abs(offset) - hoursOff * 3600) / 60;
            return localNow.toString(Qt::ISODate).append(qsl("%1%2:%3")
                                                                 .arg(offset >= 0 ? QLatin1Char('+') : QLatin1Char('-'))
                                                                 .arg(hoursOff, 2, 10, QLatin1Char('0'))
                                                                 .arg(minutesOff, 2, 10, QLatin1Char('0')));
        }
        return localNow.toString(Qt::ISODate).append(qsl("+00:00"));
#else
        auto localNow = QDateTime::currentDateTime();
        const int offset = localNow.offsetFromUtc();
        localNow.setOffsetFromUtc(offset);
        return localNow.toString(Qt::ISODate);
#endif
    }

    // Return a new QString with path made absolute, resolved against base and cleaned if it was relative
    // Returns path unchanged if it was already absolute or an empty string
    static QString pathResolveRelative(const QString& path, const QString& base)
    {
        if (path.size() == 0) {
            return path;
        }
        if (QDir::isAbsolutePath(path)) {
            return path;
        }
        return QDir::cleanPath(base + "/" + path);
    }

    // Sanitize a string for safe use as filename/path component
    // Replaces filesystem-unsafe characters with underscores and limits length
    static QString sanitizeForPath(const QString& input)
    {
        QString sanitized = input;
        // Replace filesystem-unsafe characters with underscores
        sanitized.replace(QRegularExpression(R"([/\\:*?"<>|])"), "_");
        // Limit length to prevent filesystem issues
        if (sanitized.length() > 50) {
            sanitized = sanitized.left(50);
        }
        return sanitized;
    }

    // Position a dialog on the same screen as its parent window
    // This improves multi-monitor UX by keeping dialogs with their parent windows
    static void positionDialogOnParentScreen(QWidget* dialog, QWidget* parent)
    {
        if (!dialog || !parent) {
            return;
        }

        // Get the screen containing the parent window
        // Use mapToGlobal to get the actual screen position of the parent widget
        QPoint parentPos = parent->mapToGlobal(parent->rect().center());
        const QScreen* parentScreen = QApplication::screenAt(parentPos);
        if (!parentScreen) {
            // Fallback to parent's screen property if screenAt fails
            parentScreen = parent->screen();
        }

        if (parentScreen) {
            // Get the current screen of the dialog to see if it needs repositioning
            // Use the dialog's current geometry center for more accurate screen detection
            QPoint dialogCenter = dialog->mapToGlobal(dialog->rect().center());
            const QScreen* dialogScreen = QApplication::screenAt(dialogCenter);

            // If the dialog is not visible or not yet positioned, or if it's on the wrong screen,
            // then reposition it. This handles cases where the dialog retains old positions.
            if (!dialog->isVisible() || !dialogScreen || dialogScreen != parentScreen) {
                centerDialogOnScreen(dialog, parentScreen);
            }
        }
    }

    // Position a dialog on the same screen as the active profile's console
    // This version considers the actual console widget position for better accuracy
    static void positionDialogOnActiveProfileScreen(QWidget* dialog, QWidget* parentWindow, QWidget* activeConsole)
    {
        if (!dialog) {
            return;
        }

        // Prefer the active console position if available, otherwise fall back to parent window
        QWidget* referenceWidget = activeConsole ? activeConsole : parentWindow;
        if (referenceWidget) {
            positionDialogOnParentScreen(dialog, referenceWidget);
        }
    }

    // Force reposition a dialog on the specified screen, regardless of current position
    // This is useful for singleton dialogs that may retain old positions
    static void forceRepositionDialogOnParentScreen(QWidget* dialog, QWidget* parent)
    {
        if (!dialog || !parent) {
            return;
        }

        // Get the screen containing the parent window
        QPoint parentPos = parent->mapToGlobal(parent->rect().center());
        const QScreen* parentScreen = QApplication::screenAt(parentPos);
        if (!parentScreen) {
            parentScreen = parent->screen();
        }

        if (parentScreen) {
            // Always reposition, regardless of current dialog position
            centerDialogOnScreen(dialog, parentScreen);
        }
    }

    // Position a dialog in the center of the specified screen
    static void centerDialogOnScreen(QWidget* dialog, const QScreen* screen)
    {
        if (!dialog || !screen) {
            return;
        }

        const QRect screenGeometry = screen->availableGeometry();

        // Ensure dialog has a size first
        if (dialog->size().isEmpty()) {
            dialog->adjustSize();
        }

        // Calculate center position
        const QSize dialogSize = dialog->size();
        const QPoint centerPoint = screenGeometry.center();
        const QPoint newPos(
            centerPoint.x() - dialogSize.width() / 2,
            centerPoint.y() - dialogSize.height() / 2);

        // Ensure dialog stays within screen bounds
        QPoint constrainedPos = newPos;
        constrainedPos.setX(qMax(screenGeometry.left(),
                                qMin(newPos.x(), screenGeometry.right() - dialogSize.width())));
        constrainedPos.setY(qMax(screenGeometry.top(),
                                qMin(newPos.y(), screenGeometry.bottom() - dialogSize.height())));

        dialog->move(constrainedPos);
    }
};

#endif // UPDATER_H
