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

#include "PackageItemDelegate.h"

#include "utils.h"

#include <QFontMetrics>
#include <QIcon>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QWidget>

PackageItemDelegate::PackageItemDelegate(QObject* parent)
: QStyledItemDelegate(parent)
{
    restyle(uiDesign::themeTokens());
}

void PackageItemDelegate::restyle(const uiDesign::ThemeTokens& tokens)
{
    mNameInk = tokens.text;
    mSummaryInk = tokens.mutedText;
    // Both lines of a chosen row take the wash's ink: only the name lit would read as half chosen
    mChosenInk = tokens.accentText;
    mAccentBar = tokens.accent;
    mCaptionSize = uiDesign::typeSize(uiDesign::TypeStep::Caption);

    // Walked twice: a blue readable on the pane can be lost on the accent wash
    // itemRowStyleSheet() fills a chosen row with
    const QColor okTone = uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage);
    mVersionInk = uiDesign::readableOn(tokens.pane, okTone, tokens.text, uiDesign::scmTextMinimumRatio);
    mChosenVersionInk = uiDesign::readableOn(uiDesign::blend(tokens.pane, tokens.accent, uiDesign::scmAccentWashStrength), okTone, tokens.text, uiDesign::scmTextMinimumRatio);

    // At the owner's device pixel ratio, or the glyph is blown up from a 16px square
    const auto* pOwner = qobject_cast<const QWidget*>(parent());
    const qreal ratio = pOwner ? pOwner->devicePixelRatioF() : 1.0;
    const auto glyphInked = [ratio](const QColor& colour) {
        QPixmap glyph =
                uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/packages-package.svg")), colour).scaled(QSize(cIconSize, cIconSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        glyph.setDevicePixelRatio(ratio);
        return glyph;
    };
    mQuietGlyph = glyphInked(mSummaryInk);
    mChosenGlyph = glyphInked(mChosenInk);
}

void PackageItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    // paint() draws the text and icon itself; the base paints only the sheet's row surface
    option->text.clear();
    option->icon = QIcon();
    option->features &= ~QStyleOptionViewItem::HasDecoration;
}

void PackageItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid()) {
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);

    const bool chosen = option.state & QStyle::State_Selected;
    const QString name = index.data(Qt::DisplayRole).toString();
    const QString summary = index.data(Qt::UserRole).toString();
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    painter->save();

    const RowMetrics metrics = measureRow(option, index);
    if (icon.isNull()) {
        painter->drawPixmap(metrics.icon, chosen ? mChosenGlyph : mQuietGlyph);
    } else {
        icon.paint(painter, metrics.icon, Qt::AlignCenter);
    }

    if (metrics.textWidth <= 0) {
        painter->restore();
        return;
    }

    const QFont nameFont = option.font;
    const QFont summaryFont = captionFont(option.font);
    const QFontMetrics nameMetrics(nameFont);
    const QFontMetrics summaryMetrics(summaryFont);

    const QRect nameRect(metrics.textLeft, option.rect.top() + cRowPaddingVertical, metrics.nameWidth, nameMetrics.height());
    if (metrics.nameWidth > 0) {
        painter->setFont(nameFont);
        painter->setPen(chosen ? mChosenInk : mNameInk);
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, nameMetrics.elidedText(name, Qt::ElideRight, metrics.nameWidth));
    }

    if (!metrics.version.isEmpty()) {
        painter->setFont(summaryFont);
        painter->setPen(versionInk(chosen));
        painter->drawText(QRect(metrics.versionRect.left(), nameRect.top(), metrics.versionRect.width(), nameRect.height()), Qt::AlignRight | Qt::AlignVCenter, metrics.version);
    }

    if (!summary.isEmpty()) {
        const QRect summaryRect(metrics.textLeft, nameRect.bottom() + 1 + cLineSpacing, metrics.textWidth, summaryMetrics.height());
        painter->setFont(summaryFont);
        painter->setPen(chosen ? mChosenInk : mSummaryInk);
        painter->drawText(summaryRect, Qt::AlignLeft | Qt::AlignVCenter, summaryMetrics.elidedText(summary, Qt::ElideRight, metrics.textWidth));
    }

    painter->restore();

    // After the base paint, so the bar lies over the sheet's accent wash
    if (chosen && !option.rect.isEmpty()) {
        uiDesign::paintAccentBar(painter, option.rect, mAccentBar, uiDesign::scmRadiusPanel);
    }
}

QFont PackageItemDelegate::captionFont(const QFont& rowFont) const
{
    QFont caption = rowFont;
    caption.setPointSize(mCaptionSize);
    return caption;
}

PackageItemDelegate::RowMetrics PackageItemDelegate::measureRow(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    RowMetrics metrics;

    const int leading = option.rect.left() + cRowGutter;
    metrics.icon = QRect(leading, option.rect.top() + (option.rect.height() - cIconSize) / 2, cIconSize, cIconSize);
    metrics.textLeft = metrics.icon.right() + 1 + cIconGap;
    metrics.textWidth = option.rect.right() - cRowPaddingTrailing - metrics.textLeft;
    metrics.nameWidth = metrics.textWidth;
    if (metrics.textWidth <= 0) {
        return metrics;
    }

    const QString version = index.data(cPackageVersionRole).toString();
    if (version.isEmpty()) {
        return metrics;
    }
    const int versionWidth = QFontMetrics(captionFont(option.font)).horizontalAdvance(version);
    metrics.version = version;
    metrics.versionRect = QRect(option.rect.right() - cRowPaddingTrailing - versionWidth, option.rect.top(), versionWidth, option.rect.height());
    metrics.nameWidth = qMax(0, metrics.textWidth - versionWidth - cVersionGap);
    return metrics;
}

QString PackageItemDelegate::nameDrawn(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const RowMetrics metrics = measureRow(option, index);
    if (metrics.nameWidth <= 0) {
        return {};
    }
    return QFontMetrics(option.font).elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, metrics.nameWidth);
}

QSize PackageItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    // The version rides the name's own line, so it asks the row for no height
    const int textHeight = QFontMetrics(option.font).height() + cLineSpacing + QFontMetrics(captionFont(option.font)).height();
    return QSize(option.rect.width(), qMax(textHeight, cIconSize) + 2 * cRowPaddingVertical);
}
