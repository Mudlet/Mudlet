
#ifndef _TMatchState_H_
#define _TMatchState_H_

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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

class TMatchState
{
public:
    
    TMatchState( int NumberOfConditions, int delta )
    {
        mNumberOfConditions = NumberOfConditions;
        mNextCondition = 1; // first condition was true when the state was created
        mDelta = delta;
        mLineCount = 1;
        mSpacer = 0;
    }
    
    TMatchState( const TMatchState &ms )
    {
        mNumberOfConditions = ms.mNumberOfConditions;
        mNextCondition = ms.mNextCondition;
        mDelta = ms.mDelta;
        mLineCount = ms.mLineCount;
    }
    
    int  nextCondition() { return mNextCondition; }
    void conditionMatched(){ mNextCondition++; }
    bool isComplete() 
    { 
        if( mNextCondition >= mNumberOfConditions )
        {
            return true;
        } 
        else
        {
            return false; 
        }
    }
    void newLineArrived(){ mLineCount++; }
    bool newLine() 
    { 
        if( mLineCount > mDelta )
        {
            return false;
        }
        else
        {
            return true; 
        }
    }

    bool lineSpacerMatch( int lines )
    {
        if( mSpacer >= lines )
        {
            mSpacer = 0;
            return true;
        }
        else
        {
            mSpacer++;
            return false;
        }
    }

    int                                 mSpacer;
    std::list< std::list<std::string> > multiCaptureList;
    std::list< std::list<int> >         multiCapturePosList;
    int                                 mNumberOfConditions;
    int                                 mNextCondition;
    int                                 mLineCount;
    int                                 mDelta;
};

#endif

