#ifndef MUDLET_AUTOLOGINDELAYSTESTHELPER_H
#define MUDLET_AUTOLOGINDELAYSTESTHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QSettings>
#include <QVariant>

#include "mudlet.h"
#include "utils.h"

// A test that waits for the timer-driven auto-login has to shorten its two delays, which live in a
// QSettings file shared by every case in the binary - so they have to go back however a QVERIFY
// leaves the test body.
class ScopedAutoLoginDelays
{
public:
    ScopedAutoLoginDelays(int usernameMs, int passwordMs)
    : mpSettings(mudlet::getQSettings())
    , mSavedUsername(mpSettings->value(qsl("autoLoginUsernameDelay")))
    , mSavedPassword(mpSettings->value(qsl("autoLoginPasswordDelay")))
    {
        mpSettings->setValue(qsl("autoLoginUsernameDelay"), usernameMs);
        mpSettings->setValue(qsl("autoLoginPasswordDelay"), passwordMs);
    }

    ~ScopedAutoLoginDelays()
    {
        restore(qsl("autoLoginUsernameDelay"), mSavedUsername);
        restore(qsl("autoLoginPasswordDelay"), mSavedPassword);
    }

private:
    void restore(const QString& key, const QVariant& saved) { saved.isValid() ? mpSettings->setValue(key, saved) : mpSettings->remove(key); }

    QSettings* mpSettings;
    QVariant mSavedUsername;
    QVariant mSavedPassword;
};

#endif // MUDLET_AUTOLOGINDELAYSTESTHELPER_H
