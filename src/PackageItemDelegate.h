#ifndef MUDLET_PACKAGEITEMDELEGATE_H
#define MUDLET_PACKAGEITEMDELEGATE_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com         *
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

#include "uiDesign.h"

#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QStyledItemDelegate>

// The row's surface (corner, hover and accent wash) is uiDesign::itemRowStyleSheet() on the
// list; this draws only what stands on it, plus the accent bar no stylesheet rule can shape.
class PackageItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PackageItemDelegate(QObject* parent = nullptr);

    // Qt::UserRole is the row's summary; left unset, no version is drawn
    static constexpr int cPackageVersionRole = Qt::UserRole + 1;

    void restyle(const uiDesign::ThemeTokens& tokens);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // Public for tests, which compare the pixels of a grab's version against it
    [[nodiscard]] QColor versionInk(const bool chosen) const { return chosen ? mChosenVersionInk : mVersionInk; }

    // Public for tests: the only way to ask whether the version elided the name
    [[nodiscard]] QString nameDrawn(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    // Public because itemRowStyleSheet() reserves this same gutter for the bar paint() draws
    static constexpr int cRowGutter = uiDesign::scmAccentBarWidth + 5;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;

private:
    // Shared by paint() and nameDrawn(), so what a test reads is what is painted
    struct RowMetrics
    {
        QRect icon;
        int textLeft = 0;
        int textWidth = 0;
        int nameWidth = 0;
        QString version;
        QRect versionRect;
    };
    [[nodiscard]] RowMetrics measureRow(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    [[nodiscard]] QFont captionFont(const QFont& rowFont) const;

    static constexpr int cRowPaddingVertical = 6;
    static constexpr int cRowPaddingTrailing = 8;
    static constexpr int cLineSpacing = 2;
    // The icon size the .ui file gives the list
    static constexpr int cIconSize = 16;
    static constexpr int cIconGap = 8;
    static constexpr int cVersionGap = cIconGap;

    QColor mNameInk;
    QColor mSummaryInk;
    QColor mChosenInk;
    QColor mAccentBar;
    QColor mVersionInk;
    QColor mChosenVersionInk;
    QPixmap mQuietGlyph;
    QPixmap mChosenGlyph;
    int mCaptionSize = 0;
};

#endif // MUDLET_PACKAGEITEMDELEGATE_H
