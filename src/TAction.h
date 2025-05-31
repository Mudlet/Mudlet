#ifndef MUDLET_TACTION_H
#define MUDLET_TACTION_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2020-2022 by Stephen Lyons                        *
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


#include "Tree.h"

#include "pre_guard.h"
#include <QColor>
#include <QDebug>
#include <QIcon>
#include <QObject>
#include <QPointer>
#include "post_guard.h"

class EAction;
class Host;
class mudlet;
class TEasyButtonBar;
class TFlipButton;
class TLuaInterpreter;
class TToolBar;

class QMenu;


class TAction : public Tree<TAction>, public QObject
{
    friend class XMLexport;
    friend class XMLimport;

public:
    virtual ~TAction();
    TAction(TAction* parent, Host* pHost);
    TAction(const QString& name, Host* pHost);
    void compileAll();
    QString getName() const { return mName; }
    void setName(const QString& name);
    void setButtonRotation(int rotation) { if (rotation != mButtonRotation) { setDataChanged(); mButtonRotation = rotation; } }
    int getButtonRotation() const { return mButtonRotation; }
    void setButtonColumns(int columns) { if (columns != mButtonColumns) { setDataChanged(); mButtonColumns = columns; } }
    int getButtonColumns() const { return mButtonColumns; }
    bool getButtonFlat() const { return mButtonFlat; }
    void setButtonFlat(bool flat) { if (flat != mButtonFlat) { setDataChanged(); mButtonFlat = flat; } }

    void setSizeX(int size) { if (size != mSizeX) { setDataChanged(); mSizeX = size; } }
    int getSizeX() const { return mSizeX; }
    void setSizeY(int size) { if (size != mSizeY) { setDataChanged(); mSizeY = size; } }
    int getSizeY() const { return mSizeY; }

    void fillMenu(TEasyButtonBar* pT, QMenu* menu);
    void compile();
    bool compileScript();
    void execute();
    QString getIcon() const { return mIcon; }
    void setIcon(const QString& icon) { if (icon != mIcon) { mIcon = icon; } }
    QString getScript() const { return mScript; }
    bool setScript(const QString& script);
    QString getCommandButtonUp() const { return mCommandButtonUp; }
    void setCommandButtonUp(const QString& cmd) { if (cmd != mCommandButtonUp) { setDataChanged(); mCommandButtonUp = cmd; } }
    void setCommandButtonDown(const QString& cmd) { if (cmd != mCommandButtonDown) { setDataChanged(); mCommandButtonDown = cmd; } }
    QString getCommandButtonDown() const { return mCommandButtonDown; }
    bool isPushDownButton() { return mIsPushDownButton; }
    void setIsPushDownButton(bool b) { if (b != mIsPushDownButton) { setDataChanged(); mIsPushDownButton = b; } }

    void setIsFolder(bool b) { if (b != isFolder()) { setDataChanged(); this->Tree::setIsFolder(b);} }

    bool registerAction();
    void insertActions(TToolBar* pT, QMenu* menu);
    void expandToolbar(TToolBar* pT);
    void insertActions(TEasyButtonBar* pT, QMenu* menu);
    void expandToolbar(TEasyButtonBar* pT);
    void setDataSaved() { if (mpParent) { mpParent->setDataSaved(); } mDataChanged = false; }
    void setDataChanged() { if (mpParent) { mpParent->setDataChanged(); } mDataChanged = true; }
    bool isDataChanged() { return mDataChanged; }
    QString packageName(TAction* pAction) const;
    QString moduleName(TAction* pAction) const;


    QPointer<TToolBar> mpToolBar;
    QPointer<TEasyButtonBar> mpEasyButtonBar;
    QPointer<EAction> mpEButton;
    QPointer<TFlipButton> mpFButton;
    // The following was an int but there was confusion over:
    // EITHER: "1" = released/unclicked/up & "2" = pressed/clicked/down
    // OR:     "1" = pressed/clicked/down  & "0" = released/unclicked/up
    // The Wiki says it should be "1" and "2" but the code sort of did "0"/"1"
    // in some places.
    // Now uses a boolean:
    // "true" = pressed/clicked/down & "false" = released/unclicked/up
    bool mButtonState = false;
    int mPosX = 0;
    int mPosY = 0;
    // THIS class uses 0 = horizontal, 1 = vertical; c.f. TFlipButton class
    // which uses Qt::Orientation enum for the same thing:
    int mOrientation = 0;
    // 0 to 3 are only applicable to the Easy Button Bar buttons/menus (around
    // edge of main console:
    // 0 = Top "Toolbar" (Easy Button Bar)
    // 2 = Left "Toolbar" (Easy Button Bar)
    // 3 = Left "Toolbar" (Easy Button Bar)
    // 4 = Dockable/floating Toolbar
    int mLocation = 0;
    bool mIsPushDownButton = false;

    bool mNeedsToBeCompiled = true;
    QString mIcon;
    QIcon mIconPix;

    // 0 = Horizontal
    // 1 = Vertical
    // 2 = Vertical + Mirrored
    int mButtonRotation = 0;
    int mButtonColumns = 1;
    // Not currently user accessible but was previously and maintained in game
    // saves - and applied to buttons when drawn:
    bool mButtonFlat = false;
    int mSizeX = 0;
    int mSizeY = 0;
    // Not currently user accessible but was previously and maintained in game
    // saves - and applied to buttons when drawn:
    bool mUseCustomLayout = false;
    QString css;
    QPointer<Host> mpHost;
    bool exportItem = true;
    bool mModuleMasterFolder = false;
    Qt::DockWidgetArea mToolbarLastDockArea = Qt::LeftDockWidgetArea;
    bool mToolbarLastFloatingState = true;

private:
    TAction() = default;

    QString mName;
    QString mCommandButtonUp;
    QString mCommandButtonDown;
    QString mScript;
    QString mFuncName;
    bool mModuleMember = false;
    bool mDataChanged = true;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const TAction* action)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "TAction(" << action->getName() << ")";
    debug.nospace() << ", commandButtonUp=" << action->getCommandButtonUp();
    debug.nospace() << ", commandButtonDown=" << action->getCommandButtonDown();
    debug.nospace() << ", script=" << action->getScript();
    debug.nospace() << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TACTION_H
