#ifndef MUDLET_TACTION_H
#define MUDLET_TACTION_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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
#include <QIcon>
#include "post_guard.h"

class Host;
class mudlet;
class TEasyButtonBar;
class TFlipButton;
class TLuaInterpreter;
class TToolBar;

class QMenu;


class TAction : public Tree<TAction>, QObject
{
    friend class XMLexport;
    friend class XMLimport;

public:


    virtual          ~TAction();
                     TAction( TAction * parent, Host * pHost );
                     TAction( QString name, Host * pHost );
    void             compileAll();
    QString          getName()                                 { return mName; }
    void             setName( QString name )                   { mName = name; }
    void             setButtonColor( QColor c )                { mButtonColor = c; }
    QColor           getButtonColor()                          { return mButtonColor; }
    void             setButtonRotation( int r )                { mButtonRotation = r; }
    int              getButtonRotation()                       { return mButtonRotation; }
    void             setButtonColumns( int c )                 { mButtonColumns = c; }
    int              getButtonColumns()                        { return mButtonColumns; }
    bool             getButtonFlat()                           { return mButtonFlat; }
    void             setButtonFlat( bool flat )                { mButtonFlat = flat; }

    void             setSizeX( int s )                         { mSizeX = s; }
    int              getSizeX()                                { return mSizeX; }
    void             setSizeY( int s )                         { mSizeY = s; }
    int              getSizeY()                                { return mSizeY; }

    void             fillMenu( TEasyButtonBar * pT, QMenu * menu );
    void             compile();
    bool             compileScript();
    void            execute();
    QString         getIconPathFileName( const bool isToForceRelative = false ) const;
    void            setIconPathFileName( const QString & icon ) { mIcon = icon; }
    QString          getScript()                               { return mScript; }
    bool             setScript( QString & script );
    QString          getCommandButtonUp()                      { return mCommandButtonUp; }
    void             setCommandButtonUp( QString cmd )         { mCommandButtonUp = cmd; }
    void             setCommandButtonDown( QString command )   { mCommandButtonDown = command; }
    QString          getCommandButtonDown()                    { return mCommandButtonDown; }
    bool             isFolder()                                { return mIsFolder; }
    bool             isPushDownButton()                        { return mIsPushDownButton; }
    void             setIsPushDownButton( bool b )             { mIsPushDownButton = b; }
    void             setIsFolder( bool b )                     { mIsFolder = b; }
    bool             registerAction();
    void             insertActions( mudlet * pMainWindow,
                                    TToolBar * pT,
                                    QMenu * menu );
    void             expandToolbar( mudlet * pMainWindow,
                                    TToolBar * pT,
                                    QMenu * menu );
    void             insertActions( mudlet * pMainWindow,
                                    TEasyButtonBar * pT,
                                    QMenu * menu );
    void             expandToolbar( mudlet * pMainWindow,
                                    TEasyButtonBar * pT,
                                    QMenu * menu );
    TToolBar *       mpToolBar;
    TEasyButtonBar * mpEasyButtonBar;
    int              mButtonState;
    int              mPosX;
    int              mPosY;
    int              mOrientation;
    int              mLocation;
    QString          mName;
    QString          mCommandButtonUp;
    QString          mCommandButtonDown;
    QRegExp          mRegex;
    QString          mScript;
    bool             mIsPushDownButton;
    bool             mIsFolder;

    bool             mNeedsToBeCompiled;
// Not currently used:    QIcon            mIconPix;

    int              mButtonRotation;
    int              mButtonColumns;
    bool             mButtonFlat;
    int              mSizeX;
    int              mSizeY;
    bool             mIsLabel;
    bool             mUseCustomLayout;
    QString          css;
    QColor           mButtonColor;
    Host *           mpHost;
    bool             exportItem;
    bool            mModuleMasterFolder;

private:
                    TAction(){}


    QString         mIcon;
    QString          mFuncName;
    bool                  mModuleMember;

};

#endif // MUDLET_TACTION_H
