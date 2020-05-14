#ifndef MUDLET_TENTITYHANDLER_H
#define MUDLET_TENTITYHANDLER_H
/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TEntityResolver.h"
// Handles entity processing state and conversion of simple standard entities such as &gt; &lt; &amp; and &quot;
class TEntityHandler
{
private:
    const TEntityResolver& mpEntityResolver;

    QString mCurrentEntity;
    bool mIsResolved;
    char mResult;

public:
    TEntityHandler() : TEntityHandler(TEntityResolver::scmDefaultResolver) {}
    explicit TEntityHandler(const TEntityResolver& pResolver) : mpEntityResolver(pResolver), mIsResolved(false), mResult(0) {}

    bool handle(char ch);
    void reset();

    bool isEntityResolved() const;
    char getResultAndReset();
};

#endif //MUDLET_TENTITYHANDLER_H
