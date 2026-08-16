#ifndef MUDLET_PROFILETESTHELPER_H
#define MUDLET_PROFILETESTHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <chrono>

#include <QApplication>
#include <QSignalSpy>
#include <QWidget>
#include <QtTest/QtTest>

#include "Host.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

// Creating a profile by driving the connection dialog is the first thing nearly
// every functional test does, and it used to be copied into each of them as a
// QTimer::singleShot lambda that stepped through the dialog on fixed 100ms
// sleeps. That cost two intermittent aborts of the whole test executable:
//
//  - focus could still be elsewhere when the sleep expired, and
//    QTest::keyClicks(nullptr, ...) trips QTEST_ASSERT inside QtTest, which is
//    a qFatal() rather than a test failure;
//  - the interaction ran inside the very event loop the caller was waiting on
//    with QSignalSpy::wait(), so while it was mid-flight in its own nested
//    QTest::qWait() loops the caller's timeout could not take effect - a
//    one-second wait was seen running for the full five-minute QtTest watchdog.
//
// Waiting for the state each step actually needs removes both: nothing is ever
// handed a null widget, and every wait here returns a bool at its own deadline
// instead of blocking someone else's. See issue #9973.
namespace TestProfile {

// Waits until 'target' is the focused widget. Naming the widget matters rather
// than merely waiting for focus to move: tests that create a profile more than
// once per process can arrive with focus already sitting on the field about to
// be typed into, and a wait for focus to *change* never returns for those.
inline bool waitForFocus(QWidget* target, const std::chrono::milliseconds timeout)
{
    return QTest::qWaitFor([target]() { return QApplication::focusWidget() == target; }, timeout);
}

// The dialog's "Connect" button, which is its default button - so a Return in
// any of the fields activates it, but only once it is enabled. Both it and
// "Offline" carry AcceptRole, so the default flag is what tells them apart.
inline QPushButton* connectButton(dlgConnectionProfiles* dialog)
{
    const auto buttons = dialog->dialog_buttonbox->buttons();
    for (QAbstractButton* button : buttons) {
        auto* pushButton = qobject_cast<QPushButton*>(button);
        if (pushButton && pushButton->isDefault()) {
            return pushButton;
        }
    }
    return nullptr;
}

// Waits for 'field' to take focus, types into it, then tabs on to the next one.
// Typing into the focused widget rather than straight into 'field' keeps the
// dialog seeing exactly the interaction a user would produce, selection state
// included.
inline bool typeFieldAndAdvance(QWidget* field, const QString& text, const std::chrono::milliseconds timeout)
{
    if (!field || !waitForFocus(field, timeout)) {
        return false;
    }
    QTest::keyClicks(field, text);
    QTest::keyClick(field, Qt::Key_Tab);
    return true;
}

// Drives the connection dialog to create and open a profile. Returns the loaded
// Host, or nullptr having warned about the step that timed out - callers should
// QVERIFY the result rather than dereferencing it.
inline Host* create(const QString& profileName,
                    const QString& address,
                    const QString& port,
                    const std::chrono::milliseconds timeout = std::chrono::seconds(15))
{
    mudlet::self()->startAutoLogin({});

    // Existence is the condition, not visibility: on a pristine config Mudlet
    // puts its first-run tutorial up instead of the profile dialog, which leaves
    // the dialog built but never shown. QTest delivers events to it either way,
    // and the focus waits below are what actually confirm it is driveable.
    if (!QTest::qWaitFor(
                []() {
                    dlgConnectionProfiles* dialog = mudlet::self()->mpConnectionDialog;
                    return dialog != nullptr && dialog->new_profile_button != nullptr && dialog->profile_name_entry != nullptr;
                },
                timeout)) {
        qWarning() << "TestProfile::create() - the connection dialog was never built";
        return nullptr;
    }

    dlgConnectionProfiles* dialog = mudlet::self()->mpConnectionDialog;
    QTest::mouseClick(dialog->new_profile_button, Qt::LeftButton);

    if (!typeFieldAndAdvance(dialog->profile_name_entry, profileName, timeout)) {
        qWarning() << "TestProfile::create() - the profile name field never took focus";
        return nullptr;
    }
    if (!typeFieldAndAdvance(dialog->host_name_entry, address, timeout)) {
        qWarning() << "TestProfile::create() - the address field never took focus";
        return nullptr;
    }
    if (!waitForFocus(dialog->port_entry, timeout)) {
        qWarning() << "TestProfile::create() - the port field never took focus";
        return nullptr;
    }
    QTest::keyClicks(dialog->port_entry, port);

    // Every field is filled, but the port is validated asynchronously and the
    // Connect button stays disabled until that has run - a Return arriving
    // first does nothing at all. This is the state the fixed sleep this
    // replaces was really waiting for.
    if (!QTest::qWaitFor(
                []() {
                    QPushButton* button = connectButton(mudlet::self()->mpConnectionDialog);
                    return button != nullptr && button->isEnabled();
                },
                timeout)) {
        qWarning() << "TestProfile::create() - the Connect button never became enabled";
        return nullptr;
    }

    // Connect the spy before the click that loads the profile, so the signal
    // cannot arrive before there is anything listening for it
    QSignalSpy profileLoaded(mudlet::self(), &mudlet::signal_profileLoaded);
    QTest::mouseClick(connectButton(mudlet::self()->mpConnectionDialog), Qt::LeftButton);
    // The profile usually loads synchronously inside the click, and
    // QSignalSpy::wait() only returns for a signal that arrives while it is
    // waiting - so check what the spy already holds before waiting for more.
    // The version this replaces never had to: its click ran inside the wait.
    if (profileLoaded.isEmpty() && !profileLoaded.wait(timeout)) {
        qWarning() << "TestProfile::create() - the profile did not finish loading";
        return nullptr;
    }

    return mudlet::self()->getActiveHost();
}

} // namespace TestProfile

#endif // MUDLET_PROFILETESTHELPER_H
