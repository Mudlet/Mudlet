#ifndef MUDLET_TWINDOWNOTIFIER_H
#define MUDLET_TWINDOWNOTIFIER_H

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

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

// Tells, once a turn, that a call may have changed the windows the null view keeps for a profile's scripts, and which
// of its consoles were written to. A listener compares the windows it reads then with what it read before, so a call
// that changed nothing tells nothing.
class TWindowNotifier : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    void note()
    {
        if (mPending) {
            return;
        }
        mPending = true;
        QMetaObject::invokeMethod(this, &TWindowNotifier::tellWindows, Qt::QueuedConnection);
    }

    // The text of the console of that name may have changed
    void noteText(const QString& name)
    {
        if (mTextChanged.isEmpty()) {
            QMetaObject::invokeMethod(
                    this,
                    [this]() {
                        // First, so that a listener hears of a console made this turn before it hears of its text
                        tellWindows();
                        const QStringList names(mTextChanged.cbegin(), mTextChanged.cend());
                        mTextChanged.clear();
                        emit textChanged(names);
                    },
                    Qt::QueuedConnection);
        }
        mTextChanged.insert(name);
    }

signals:
    void windowsChanged();
    // In no order, each name once
    void textChanged(const QStringList& names);

private:
    void tellWindows()
    {
        if (mPending) {
            mPending = false;
            emit windowsChanged();
        }
    }

    bool mPending = false;
    QSet<QString> mTextChanged;
};

#endif // MUDLET_TWINDOWNOTIFIER_H
