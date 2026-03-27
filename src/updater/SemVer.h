/***************************************************************************
 *   Copyright (C) 2017 by Philipp Medien - hello@dblsqd.com               *
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
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

#ifndef DBLSQD_SEMVER_H
#define DBLSQD_SEMVER_H

#include <QString>

namespace dblsqd {

class SemVer
{
public:
    explicit SemVer(const QString& version);

    bool operator<(const SemVer& other) const;

    bool isValid() const;

private:
    QString mOriginal;
    int mMajor{0};
    int mMinor{0};
    int mPatch{0};
    QString mPrerelease;
    QString mBuild;
    bool mValid{false};

    static QString getRegExp();
};

} // namespace dblsqd

#endif // DBLSQD_SEMVER_H
