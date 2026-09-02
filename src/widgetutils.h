#ifndef MUDLET_WIDGETUTILS_H
#define MUDLET_WIDGETUTILS_H

/***************************************************************************
 *   Copyright (C) 2025-2026 by Vadim Peretokin                            *
 *                                          - vadim.peretokin@mudlet.org   *
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QWidget>

// Widget-level helpers, kept apart from utils.h so that the model-side code
// which needs qsl() does not drag Qt Widgets in with it.
class widgetutils
{
public:
    // Call this in the destructor of a window class that connects any of its
    // own widgets to its own slots - keep it first, so that nothing else the
    // destructor does can deliver a child's signal either.
    //
    // A visible window is taken off the screen while the base-class
    // destructors unwind: ~QDialog hides it explicitly, and any other window
    // class gets closed by ~QWidget. That moves the keyboard focus away from
    // whichever child widget holds it, and an editing widget reacts to the
    // focus-out by emitting - QLineEdit (once its text has been touched, which
    // includes any setText()), QAbstractSpinBox and QKeySequenceEdit all emit
    // editingFinished() there. Qt then tries to deliver that to a slot of a
    // window whose derived part has already been destroyed, which aborts with
    // "Called object is not of the correct type (class destructor may have
    // already run)" (#9574). In a release build the assert is compiled out and
    // the slot runs against destroyed members instead.
    //
    // A window that is being destroyed cannot do anything useful with a
    // signal from its own widgets, so every one of them is severed rather
    // than just the widget types that emit during teardown today. Note that
    // this only reaches connections whose receiver is the window: a
    // connect(child, &Signal, [this]{...}) written without a context object
    // survives it and brings the crash back, so always pass the context:
    static void disconnectChildSignals(QWidget* window)
    {
        for (QObject* child : window->findChildren<QObject*>()) {
            QObject::disconnect(child, nullptr, window, nullptr);
        }
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
        const QPoint newPos(centerPoint.x() - dialogSize.width() / 2, centerPoint.y() - dialogSize.height() / 2);

        // Ensure dialog stays within screen bounds
        QPoint constrainedPos = newPos;
        constrainedPos.setX(qMax(screenGeometry.left(), qMin(newPos.x(), screenGeometry.right() - dialogSize.width())));
        constrainedPos.setY(qMax(screenGeometry.top(), qMin(newPos.y(), screenGeometry.bottom() - dialogSize.height())));

        dialog->move(constrainedPos);
    }
};

#endif // MUDLET_WIDGETUTILS_H
