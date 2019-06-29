#ifndef MUDLET_TKEY_H
#define MUDLET_TKEY_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2019 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "Tree.h"

#include "pre_guard.h"
#include <QPointer>
#include <QRegularExpression>
#include "post_guard.h"

class Host;


class TKey : public Tree<TKey>
{
    friend class XMLexport;
    friend class XMLimport;

public:
    virtual ~TKey();
    TKey(TKey* parent, Host* pHost);
    TKey(QString name, Host* pHost);
    void compileAll();
    QString getName() {
        return mName;
    }
    void setName(const QString & name);
    int getKeyCode() {
        return mKeyCode;
    }
    void setKeyCode(int code) {
        mKeyCode = code;
    }
    QPair<Qt::KeyboardModifiers, Qt::KeyboardModifiers> getKeyModifiers() const {
        return qMakePair(mPresentModifiers, mAbsentModifiers);
    }
    void setKeyModifiers(const QPair<Qt::KeyboardModifiers, Qt::KeyboardModifiers> modifiers) {
        mPresentModifiers = modifiers.first;
        // The bitwise logic is to prevent bits (modifiers) being placed in the
        // mAbsentKeyModifiers member if they are present in the
        // mPresentKeyModifiers one:
        mAbsentModifiers = modifiers.second &~(modifiers.first);
    }
    void enableKey(const QString& name);
    void disableKey(const QString& name);
    void compile();
    bool compileScript();
    void execute();
    QString getScript() {
        return mScript;
    }
    bool setScript(QString& script);
    void setCommand(QString command) {
        mCommand = command;
    }
    QString getCommand() {
        return mCommand;
    }

    bool match(const int, const Qt::KeyboardModifiers modifiers, const bool);
    bool registerKey();

    bool exportItem;
    bool mModuleMasterFolder;

    static const quint32 scmPresentModifierMask = 0x7E000000;
    static const quint32 scmAbsentModifierMask  = 0x007E0000;

private:
    TKey() = default;

    QString mName;
    QString mCommand;

    /*
     * Qt::NoModifier          0x00000000 No modifier key is pressed.
     * Qt::ShiftModifier       0x02000000 A Shift key on the keyboard is pressed.
     * Qt::ControlModifier     0x04000000 A Ctrl key on the keyboard is pressed.
     * Qt::AltModifier         0x08000000 An Alt key on the keyboard is pressed.
     * Qt::MetaModifier        0x10000000 A Meta key on the keyboard is pressed.
     * Qt::KeypadModifier      0x20000000 A keypad button is pressed.
     * Qt::GroupSwitchModifier 0x40000000 (X11-only) Group switch modifier is present (maybe associated with AltGr key on PCs)
     * Around 3.20.0 we use some otherwise unused bits to represent the inverse
     * when saving/loading shift the mAbsentModifiers bits left/right by 8
     * places and combine with the mPresentModifiers:
     * NoShiftModifier         0x00020000 A Shift key on the keyboard is NOT pressed.
     * NoControlModidier       0x00040000 A Ctrl key on the keyboard is NOT preseed.
     * NoAltModifier           0x00080000 An Alt key on the keyboard is pressed.
     * NoMetaModifier          0x00100000 A Meta key on the keyboard is pressed.
     * NoKeypadModifier        0x00200000 A keypad button is NOT pressed.
     * NoGroupSwitchModifier   0x00400000 (X11-only) Group switch modifier is NOT present
     */

    int mKeyCode;
    // The modifiers must be present for the key to fire:
    Qt::KeyboardModifiers mPresentModifiers;
    // The modifiers must NOT be present for the key to fire - should the
    // modifier be present in BOTH then ignore it in this one:
    Qt::KeyboardModifiers mAbsentModifiers;

    QString mRegexCode;
    QRegularExpression mRegex;
    QString mScript;
    QString mFuncName;
    QPointer<Host> mpHost;
    bool mNeedsToBeCompiled;
    bool mModuleMember;
};

#endif // MUDLET_TKEY_H
