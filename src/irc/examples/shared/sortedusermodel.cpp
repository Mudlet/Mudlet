/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "sortedusermodel.h"
#include "usermodel.h"

SortedUserModel::SortedUserModel(const QString& prefixes, UserModel* userModel)
    : QSortFilterProxyModel(userModel), pfx(prefixes)
{
    setSourceModel(userModel);
    sort(0, Qt::AscendingOrder);
    setDynamicSortFilter(true);
}

bool SortedUserModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    QString u1 = sourceModel()->data(left, Qt::DisplayRole).toString();
    QString u2 = sourceModel()->data(right, Qt::DisplayRole).toString();

    const int i1 = pfx.indexOf(u1.at(0));
    const int i2 = pfx.indexOf(u2.at(0));

    if (i1 >= 0 && i2 < 0)
        return true;
    if (i1 < 0 && i2 >= 0)
        return false;
    if (i1 >= 0 && i2 >= 0 && i1 != i2)
        return i1 < i2;

    return QString::localeAwareCompare(u1.toLower(), u2.toLower()) < 0;
}
