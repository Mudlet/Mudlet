#ifndef MUDLET_TKEY_H
#define MUDLET_TKEY_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018, 2020 by Stephen Lyons - slysven@virginmedia.com   *
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
    QString getName() { return mName; }
    void setName(const QString & name);
    Qt::Key getKeyCode() const { return mKeyCode; }
    void setKeyCode(const Qt::Key code) { mKeyCode = code; }
    void setKeyCode(const int codeNumber) { setKeyCode(static_cast<Qt::Key>(codeNumber)); }
    Qt::KeyboardModifiers getKeyModifiers() { return mKeyModifier; }
    void setKeyModifiers(const Qt::KeyboardModifiers code) { mKeyModifier = code; }
    void setKeyModifiers(const int codeNumber) { setKeyModifiers(static_cast<Qt::KeyboardModifiers>(codeNumber)); }
    void enableKey(const QString& name);
    void disableKey(const QString& name);
    void compile();
    bool compileScript();
    void execute();
    QString getScript() { return mScript; }
    bool setScript(const QString& script);
    void setCommand(QString command) { mCommand = command; }
    QString getCommand() { return mCommand; }

    bool match(const Qt::Key, const Qt::KeyboardModifiers, const bool);
    bool registerKey();

    bool exportItem;
    bool mModuleMasterFolder;
    bool mRegisteredAnonymousLuaFunction;

private:
    TKey() = default;

    QString mName;
    QString mCommand;

    /*
     * Qt::NoModifier      0x00000000 No modifier key is pressed.
     * Qt::ShiftModifier   0x02000000 A Shift key on the keyboard is pressed.
     * Qt::ControlModifier 0x04000000 A Ctrl key on the keyboard is pressed.
     * Qt::AltModifier     0x08000000 An Alt key on the keyboard is pressed.
     * Qt::MetaModifier    0x10000000 A Meta key on the keyboard is pressed.
     * Qt::KeypadModifier  0x20000000 A keypad button is pressed.
     */

    Qt::Key mKeyCode;
    Qt::KeyboardModifiers mKeyModifier;

    QString mRegexCode;
    QString mScript;
    QString mFuncName;
    QPointer<Host> mpHost;
    bool mNeedsToBeCompiled;
    bool mModuleMember;
};

#endif // MUDLET_TKEY_H
