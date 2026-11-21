#ifndef MUDLET_ABOUTSUPPORTERBANNER_H
#define MUDLET_ABOUTSUPPORTERBANNER_H

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

#include <QPixmap>
#include <QString>
#include <QWidget>

#include "uiDesign.h"

namespace uiDesign {

// Painted rather than lettered onto a raster frame, which cannot be translated
// and is a 1x asset on a HiDPI screen.
class AboutSupporterBanner : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(AboutSupporterBanner)
    AboutSupporterBanner(const QString& name, const bool swords, QWidget* pParent = nullptr);

    void applyTokens(const uiDesign::ThemeTokens& tokens);
    QString name() const { return mName; }
    bool swords() const { return mSwords; }

protected:
    void paintEvent(QPaintEvent* pEvent) override;

private:
    QString mName;
    bool mSwords = false;
    QPixmap mGlyph;
    uiDesign::ThemeTokens mTokens;
};

} // namespace uiDesign

#endif // MUDLET_ABOUTSUPPORTERBANNER_H
