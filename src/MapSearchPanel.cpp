/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "MapSearchPanel.h"

#include "utils.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr qreal scmCardRadius = 12.0;
// How far the shadow drops below the card
constexpr int scmShadowDrop = 4;
constexpr int scmCardPadding = 10;

QColor mixed(const QColor& from, const QColor& to, const qreal amount)
{
    return QColor::fromRgbF(static_cast<float>(from.redF() + (to.redF() - from.redF()) * amount),
                            static_cast<float>(from.greenF() + (to.greenF() - from.greenF()) * amount),
                            static_cast<float>(from.blueF() + (to.blueF() - from.blueF()) * amount));
}

QColor withAlpha(QColor color, const qreal alpha)
{
    color.setAlphaF(static_cast<float>(alpha));
    return color;
}

QString css(const QColor& color)
{
    return qsl("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

QFont scaled(const QFont& font, const qreal factor)
{
    QFont result(font);
    if (font.pointSizeF() > 0) {
        result.setPointSizeF(font.pointSizeF() * factor);
    } else {
        result.setPixelSize(std::max(1, qRound(font.pixelSize() * factor)));
    }
    return result;
}

// Drawn rather than taken from the icon theme, which many Linux desktops and every other platform lack; at
// several scales, as the panel can move to a screen with another
QIcon drawnIcon(const QColor& color, const bool cross)
{
    constexpr int size = 16;
    QIcon icon;
    for (const int pixelRatio : {1, 2, 3}) {
        QPixmap pixmap(size * pixelRatio, size * pixelRatio);
        pixmap.setDevicePixelRatio(pixelRatio);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(color, 1.6);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        if (cross) {
            painter.drawLine(QPointF(4.5, 4.5), QPointF(11.5, 11.5));
            painter.drawLine(QPointF(11.5, 4.5), QPointF(4.5, 11.5));
        } else {
            painter.drawEllipse(QRectF(2.5, 2.5, 8.0, 8.0));
            painter.drawLine(QPointF(9.8, 9.8), QPointF(13.5, 13.5));
        }
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

// Each place found on two lines: its name, with what was typed in bold, over where it is
class ResultDelegate : public QStyledItemDelegate
{
public:
    ResultDelegate(const QString& areaTag, const QString& labelTag, QObject* parent)
    : QStyledItemDelegate(parent)
    , mAreaTag(areaTag)
    , mLabelTag(labelTag)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const override
    {
        return QSize(200, QFontMetrics(option.font).height() + scmLineGap + QFontMetrics(scaled(option.font, scmDetailScale)).height() + 2 * scmRowPadding);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const QPalette colors = QApplication::palette();
        const QColor window = colors.color(QPalette::Window);
        const QColor text = colors.color(QPalette::WindowText);
        const QColor accent = colors.color(QPalette::Highlight);
        const QColor muted = mixed(text, window, 0.42);
        const bool dark = window.lightness() < 128;
        const QRectF tile = QRectF(option.rect).adjusted(1.0, 1.0, -1.0, -1.0);
        painter->setPen(Qt::NoPen);
        if (option.state & QStyle::State_Selected) {
            painter->setBrush(withAlpha(accent, dark ? 0.32 : 0.16));
            painter->drawRoundedRect(tile, 8.0, 8.0);
            painter->setBrush(accent);
            painter->drawRoundedRect(QRectF(tile.left() + 3.0, tile.top() + 9.0, 3.0, tile.height() - 18.0), 1.5, 1.5);
        } else if (option.state & QStyle::State_MouseOver) {
            painter->setBrush(withAlpha(text, dark ? 0.10 : 0.06));
            painter->drawRoundedRect(tile, 8.0, 8.0);
        }

        const auto kind = static_cast<MapSearch::Kind>(index.data(MapSearchPanel::KindRole).toInt());
        constexpr qreal iconSize = 16.0;
        const QRectF icon(option.rect.left() + 14.0, option.rect.top() + (option.rect.height() - iconSize) / 2.0, iconSize, iconSize);
        paintIcon(painter, kind, icon, index.data(MapSearchPanel::ChipColorRole).value<QColor>(), text, muted);

        int right = option.rect.right() - 12;
        const QString tag = kind == MapSearch::Kind::Area ? mAreaTag : (kind == MapSearch::Kind::Label ? mLabelTag : QString());
        if (!tag.isEmpty()) {
            const QFont tagFont = scaled(option.font, 0.8);
            const QFontMetrics tagMetrics(tagFont);
            const int height = tagMetrics.height() + 4;
            const QRectF pill(right - tagMetrics.horizontalAdvance(tag) - 14, option.rect.top() + (option.rect.height() - height) / 2.0, tagMetrics.horizontalAdvance(tag) + 14, height);
            painter->setPen(QPen(mixed(text, window, 0.72), 1.0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(pill.adjusted(0.5, 0.5, -0.5, -0.5), height / 2.0, height / 2.0);
            painter->setFont(tagFont);
            painter->setPen(muted);
            painter->drawText(pill, Qt::AlignCenter, tag);
            right = qFloor(pill.left()) - 10;
        }

        const int left = qCeil(icon.right()) + 12;
        const QFont titleFont = option.font;
        QFont matchFont = titleFont;
        matchFont.setWeight(QFont::Bold);
        const QFont detailFont = scaled(option.font, scmDetailScale);
        const QFontMetrics titleMetrics(titleFont);
        const QFontMetrics detailMetrics(detailFont);
        const int top = option.rect.top() + (option.rect.height() - titleMetrics.height() - scmLineGap - detailMetrics.height()) / 2;

        const QString title = index.data(Qt::DisplayRole).toString();
        const int start = index.data(MapSearchPanel::MatchStartRole).toInt();
        const int length = index.data(MapSearchPanel::MatchLengthRole).toInt();
        const bool matched = start >= 0 && length > 0 && start + length <= title.size();
        const QColor matchColor = mixed(accent, text, dark ? 0.15 : 0.3);
        qreal x = left;
        const qreal baseline = top + titleMetrics.ascent();
        const auto piece = [&](const QString& words, const QFont& font, const QColor& color) {
            if (words.isEmpty() || x >= right) {
                return;
            }
            const QFontMetrics metrics(font);
            const QString shown = metrics.elidedText(words, Qt::ElideRight, qFloor(right - x));
            painter->setFont(font);
            painter->setPen(color);
            painter->drawText(QPointF(x, baseline), shown);
            // Once shortened, nothing after it is drawn
            x = shown == words ? x + metrics.horizontalAdvance(shown) : right;
        };
        if (matched) {
            piece(title.left(start), titleFont, text);
            piece(title.mid(start, length), matchFont, matchColor);
            piece(title.mid(start + length), titleFont, text);
        } else {
            piece(title, titleFont, text);
        }

        painter->setFont(detailFont);
        painter->setPen(muted);
        const QString detail = detailMetrics.elidedText(index.data(MapSearchPanel::DetailRole).toString(), Qt::ElideRight, std::max(0, right - left));
        painter->drawText(QPointF(left, top + titleMetrics.height() + scmLineGap + detailMetrics.ascent()), detail);
        painter->restore();
    }

private:
    static constexpr int scmRowPadding = 7;
    static constexpr int scmLineGap = 2;
    static constexpr qreal scmDetailScale = 0.88;

    static void paintIcon(QPainter* painter, const MapSearch::Kind kind, const QRectF& icon, const QColor& chip, const QColor& text, const QColor& muted)
    {
        switch (kind) {
        case MapSearch::Kind::Room:
            painter->setPen(QPen(withAlpha(text, 0.3), 1.0));
            painter->setBrush(chip.isValid() ? chip : muted);
            painter->drawRoundedRect(icon.adjusted(1.5, 1.5, -1.5, -1.5), 4.0, 4.0);
            return;
        case MapSearch::Kind::Area:
            // A few rooms
            painter->setPen(Qt::NoPen);
            painter->setBrush(muted);
            for (int column = 0; column < 2; ++column) {
                for (int row = 0; row < 2; ++row) {
                    painter->drawRoundedRect(QRectF(icon.left() + 1.5 + column * 7.0, icon.top() + 1.5 + row * 7.0, 6.0, 6.0), 1.5, 1.5);
                }
            }
            return;
        case MapSearch::Kind::Label: {
            QPainterPath tagShape;
            tagShape.moveTo(icon.left() + 1.5, icon.top() + 3.5);
            tagShape.lineTo(icon.left() + 10.0, icon.top() + 3.5);
            tagShape.lineTo(icon.left() + 14.5, icon.top() + 8.0);
            tagShape.lineTo(icon.left() + 10.0, icon.top() + 12.5);
            tagShape.lineTo(icon.left() + 1.5, icon.top() + 12.5);
            tagShape.closeSubpath();
            painter->setPen(QPen(muted, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(tagShape);
            painter->setBrush(muted);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(icon.left() + 5.0, icon.top() + 8.0), 1.3, 1.3);
            return;
        }
        }
    }

    QString mAreaTag;
    QString mLabelTag;
};

} // namespace

MapSearchPanel::MapSearchPanel(QWidget* parent)
: QFrame(parent)
{
    setObjectName(qsl("mapSearchPanel"));
    setFrameShape(QFrame::NoFrame);
    // The card and its shadow are painted, leaving the corners to the map under them
    setAutoFillBackground(false);
    // Or a click, a drag or the wheel the panel's own widgets leave would reach the map under it
    setAttribute(Qt::WA_NoMousePropagation);

    mpEdit = new QLineEdit(this);
    //: Placeholder of the search box over the 3D map
    mpEdit->setPlaceholderText(tr("Search rooms, areas and labels…"));
    //: Tooltip of the search box over the 3D map
    mpEdit->setToolTip(tr("<p>Searches every area of the map for room names, area names, room numbers and map labels.</p>"
                          "<p>Enter flies to the place chosen, Ctrl+Enter plans a route to a room or area. Escape clears the search, and closes it once it is empty.</p>"));
    //: Name of the search box over the 3D map, read out by screen readers
    mpEdit->setAccessibleName(tr("Search the map"));
    mpEdit->setClearButtonEnabled(true);
    mpSearchIcon = mpEdit->addAction(QIcon(), QLineEdit::LeadingPosition);

    //: Toggle by the search box over the 3D map: whether map labels are searched too
    mpLabels = new QToolButton(this);
    mpLabels->setText(tr("Labels"));
    mpLabels->setCheckable(true);
    mpLabels->setChecked(true);
    // Clicked, it leaves the keys with the search box
    mpLabels->setFocusPolicy(Qt::TabFocus);
    //: Tooltip of the toggle by the search box over the 3D map
    mpLabels->setToolTip(tr("Search the text of map labels too"));
    //: Name of the toggle by the search box over the 3D map, read out by screen readers
    mpLabels->setAccessibleName(tr("Include map labels"));

    mpClose = new QToolButton(this);
    mpClose->setFocusPolicy(Qt::TabFocus);
    //: Tooltip of the button that closes the search box over the 3D map
    mpClose->setToolTip(tr("Close the search"));
    //: Name of the button that closes the search box over the 3D map, read out by screen readers
    mpClose->setAccessibleName(tr("Close the search"));

    mpList = new QListWidget(this);
    //: Name of the list of places found by the search over the 3D map, read out by screen readers
    mpList->setAccessibleName(tr("Places found"));
    mpList->setFrameShape(QFrame::NoFrame);
    mpList->setAutoFillBackground(false);
    mpList->viewport()->setAutoFillBackground(false);
    mpList->viewport()->setAttribute(Qt::WA_Hover);
    mpList->setMouseTracking(true);
    // The keys stay with the search box, which moves through the list for them
    mpList->setFocusPolicy(Qt::NoFocus);
    mpList->setContextMenuPolicy(Qt::CustomContextMenu);
    mpList->setUniformItemSizes(true);
    mpList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mpList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // As tall as its rows, up to the most it is allowed
    mpList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    //: Tag on a place found by the search over the 3D map that is a whole area
    const QString areaTag = tr("Area");
    //: Tag on a place found by the search over the 3D map that is a map label
    const QString labelTag = tr("Label");
    mpList->setItemDelegate(new ResultDelegate(areaTag, labelTag, mpList));
    mpList->hide();

    mpMessage = new QLabel(this);
    mpMessage->setTextFormat(Qt::PlainText);
    QFont headingFont = font();
    headingFont.setWeight(QFont::DemiBold);
    mpMessage->setFont(headingFont);
    mpMessage->setContentsMargins(4, 6, 4, 0);
    mpHint = new QLabel(this);
    mpHint->setTextFormat(Qt::PlainText);
    mpHint->setWordWrap(true);
    mpHint->setContentsMargins(4, 0, 4, 6);
    mpCount = new QLabel(this);
    mpCount->setFont(scaled(font(), 0.85));
    mpKeys = new QLabel(this);
    mpKeys->setFont(scaled(font(), 0.85));
    mpKeys->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    mpEdit->setFont(scaled(font(), 1.1));

    auto* top = new QHBoxLayout();
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(6);
    top->addWidget(mpEdit, 1);
    top->addWidget(mpLabels);
    top->addWidget(mpClose);
    auto* footer = new QHBoxLayout();
    footer->setContentsMargins(4, 0, 4, 0);
    footer->addWidget(mpCount);
    footer->addStretch(1);
    footer->addWidget(mpKeys);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(
            scmShadowMargin + scmCardPadding, scmShadowMargin - scmShadowDrop + scmCardPadding, scmShadowMargin + scmCardPadding, scmShadowMargin + scmShadowDrop + scmCardPadding - 2);
    layout->setSpacing(6);
    layout->addLayout(top);
    layout->addWidget(mpList, 1);
    layout->addWidget(mpMessage);
    layout->addWidget(mpHint);
    layout->addLayout(footer);

    connect(mpEdit, &QLineEdit::textChanged, this, &MapSearchPanel::queryChanged);
    connect(mpLabels, &QToolButton::toggled, this, &MapSearchPanel::queryChanged);
    connect(mpClose, &QToolButton::clicked, this, &MapSearchPanel::dismiss);
    // Clicked, not activated, which most styles keep for a double click
    connect(mpList, &QListWidget::itemClicked, this, [this]() {
        emit resultChosen(mpList->currentRow(), false);
    });
    connect(mpList, &QListWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        if (QListWidgetItem* item = mpList->itemAt(position)) {
            mpList->setCurrentItem(item);
            emit contextMenuRequested(mpList->row(item), mpList->viewport()->mapToGlobal(position));
        }
    });
    mpEdit->installEventFilter(this);
    mpList->installEventFilter(this);
    restyle();
    setResults({}, 0);
}

void MapSearchPanel::open()
{
    show();
    raise();
    mpEdit->setFocus(Qt::ShortcutFocusReason);
    mpEdit->selectAll();
}

QString MapSearchPanel::text() const
{
    return mpEdit->text();
}

bool MapSearchPanel::labelsIncluded() const
{
    return mpLabels->isChecked();
}

int MapSearchPanel::currentRow() const
{
    return mpList->currentRow();
}

int MapSearchPanel::preferredWidth() const
{
    return std::clamp(fontMetrics().averageCharWidth() * 64, 400, 720) + 2 * scmShadowMargin;
}

void MapSearchPanel::setResults(const QList<Row>& rows, const qsizetype total)
{
    const QString query = mpEdit->text().simplified();
    mpList->clear();
    for (const Row& row : rows) {
        auto* item = new QListWidgetItem(row.title, mpList);
        item->setData(DetailRole, row.detail);
        item->setData(KindRole, static_cast<int>(row.kind));
        item->setData(ChipColorRole, row.chip);
        item->setData(Qt::AccessibleTextRole, row.spoken);
        const qsizetype start = query.isEmpty() ? -1 : row.title.indexOf(query, 0, Qt::CaseInsensitive);
        item->setData(MatchStartRole, static_cast<int>(start));
        item->setData(MatchLengthRole, start < 0 ? 0 : static_cast<int>(query.size()));
    }
    mpList->setCurrentRow(rows.isEmpty() ? -1 : 0);
    constexpr qsizetype maxRowsShown = 8;
    const int rowsShown = static_cast<int>(std::min(rows.size(), maxRowsShown));
    // At most, so that a short view can squeeze it
    mpList->setMaximumHeight(rowsShown * mpList->sizeHintForRow(0) + 2 * mpList->frameWidth());
    mpList->setVisible(!rows.isEmpty());
    showState(rows.size(), total);
}

void MapSearchPanel::showState(const qsizetype shown, const qsizetype total)
{
    const QString query = mpEdit->text().simplified();
    QString heading;
    QString hint;
    if (query.isEmpty()) {
        //: Heading shown in the search over the 3D map before anything is typed
        heading = tr("Search the whole map");
        //: Shown in the search over the 3D map before anything is typed, under "Search the whole map"
        hint = tr("Type part of a room's name or its number, an area's name or a label's text.");
    } else if (!total) {
        constexpr qsizetype longest = 40;
        const QString quoted = query.size() > longest ? qsl("%1…").arg(query.left(longest)) : query;
        //: Shown in the search over the 3D map when nothing matches; %1 is what was typed
        heading = tr("No matches for “%1”").arg(quoted);
        hint = labelsIncluded()
                       //: Shown in the search over the 3D map when nothing matches, under "No matches for ..."
                       ? tr("Try fewer letters, or a room number.")
                       //: Shown in the search over the 3D map when nothing matches and map labels are left out of it
                       : tr("Map labels are left out; turn on Labels to search them too.");
    }
    mpMessage->setText(heading);
    mpHint->setText(hint);
    mpMessage->setVisible(!heading.isEmpty());
    mpHint->setVisible(!heading.isEmpty());

    if (!total) {
        mpCount->clear();
    } else if (total <= shown) {
        //: Count of the places found by the search over the 3D map
        mpCount->setText(tr("%n found", nullptr, static_cast<int>(total)));
    } else {
        //: Count of the places found by the search over the 3D map when only the best are listed; %1 is how many are listed and %n how many were found
        mpCount->setText(tr("Showing the best %1 of %n", nullptr, static_cast<int>(total)).arg(shown));
    }
    if (!shown) {
        mpKeys->setText(query.isEmpty()
                                //: Key hint at the foot of the search over the 3D map while nothing is typed
                                ? tr("Esc close")
                                //: Key hint at the foot of the search over the 3D map when nothing matches what was typed
                                : tr("Esc clear"));
        return;
    }
    //: Key hints at the foot of the search over the 3D map with places listed; %1 is the modifier key, such as Ctrl+ or ⌘
    mpKeys->setText(tr("↑↓ choose · ↵ go · %1↵ route · Esc clear").arg(QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText)));
}

void MapSearchPanel::restyle()
{
    if (mRestyling) {
        return;
    }
    mRestyling = true;
    // The application's rather than the panel's own, which the style sheets of the consoles around the map turn
    // to its background color
    const QPalette colors = QApplication::palette();
    const QColor window = colors.color(QPalette::Window);
    const QColor text = colors.color(QPalette::WindowText);
    const QColor accent = colors.color(QPalette::Highlight);
    const QColor muted = mixed(text, window, 0.42);
    const QColor line = mixed(text, window, 0.75);
    const bool dark = window.lightness() < 128;
    mpEdit->setStyleSheet(qsl("QLineEdit { background: %1; color: %2; border: 1px solid %3; border-radius: 8px; padding: 5px 4px; selection-background-color: %4; selection-color: %5; }"
                              "QLineEdit:focus { border: 1px solid %4; }")
                                  .arg(css(colors.color(QPalette::Base)), css(colors.color(QPalette::Text)), css(line), css(accent), css(colors.color(QPalette::HighlightedText))));
    mpLabels->setStyleSheet(qsl("QToolButton { background: transparent; color: %1; border: 1px solid %2; border-radius: 12px; padding: 3px 11px; }"
                                "QToolButton:hover { color: %3; border-color: %1; }"
                                "QToolButton:checked { background: %4; color: %3; border-color: %5; }"
                                "QToolButton:focus { border-color: %5; }")
                                    .arg(css(muted), css(line), css(text), css(withAlpha(accent, dark ? 0.35 : 0.18)), css(accent)));
    mpClose->setStyleSheet(qsl("QToolButton { background: transparent; border: none; border-radius: 13px; padding: 4px; }"
                               "QToolButton:hover { background: %1; }"
                               "QToolButton:focus { border: 1px solid %2; }")
                                   .arg(css(withAlpha(text, dark ? 0.14 : 0.08)), css(accent)));
    mpSearchIcon->setIcon(drawnIcon(muted, false));
    mpClose->setIcon(drawnIcon(muted, true));
    mpList->setStyleSheet(qsl("QListWidget { background: transparent; border: none; }"
                              "QScrollBar:vertical { background: transparent; width: 10px; margin: 2px 0 2px 2px; }"
                              "QScrollBar::handle:vertical { background: %1; border-radius: 4px; min-height: 24px; }"
                              "QScrollBar::handle:vertical:hover { background: %2; }"
                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }")
                                  .arg(css(withAlpha(text, dark ? 0.28 : 0.22)), css(withAlpha(text, dark ? 0.45 : 0.38))));
    const QString mutedText = qsl("QLabel { color: %1; }").arg(css(muted));
    mpCount->setStyleSheet(mutedText);
    mpKeys->setStyleSheet(mutedText);
    mpHint->setStyleSheet(mutedText);
    mpMessage->setStyleSheet(qsl("QLabel { color: %1; }").arg(css(text)));
    update();
    mRestyling = false;
}

void MapSearchPanel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF card = QRectF(rect()).adjusted(scmShadowMargin + 0.5, scmShadowMargin - scmShadowDrop + 0.5, -scmShadowMargin - 0.5, -scmShadowMargin - scmShadowDrop - 0.5);
    painter.setPen(Qt::NoPen);
    // Layered, so that it fades out to nothing; a QGraphicsDropShadowEffect would draw the whole panel off screen first
    for (int spread = scmShadowMargin; spread > 0; --spread) {
        const qreal nearness = 1.0 - static_cast<qreal>(spread) / scmShadowMargin;
        painter.setBrush(QColor(0, 0, 0, qRound(1 + 6 * nearness * nearness)));
        painter.drawRoundedRect(card.translated(0.0, scmShadowDrop).adjusted(-spread, -spread, spread, spread), scmCardRadius + spread, scmCardRadius + spread);
    }
    const QPalette colors = QApplication::palette();
    const QColor window = colors.color(QPalette::Window);
    painter.setBrush(window);
    painter.setPen(QPen(mixed(colors.color(QPalette::WindowText), window, 0.78), 1.0));
    painter.drawRoundedRect(card, scmCardRadius, scmCardRadius);
}

void MapSearchPanel::changeEvent(QEvent* event)
{
    QFrame::changeEvent(event);
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::StyleChange) {
        restyle();
    }
}

void MapSearchPanel::keyPressEvent(QKeyEvent* event)
{
    // From the toggle or the close button, which leave it to the panel
    if (event->key() == Qt::Key_Escape) {
        escape();
    } else if (!(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) && !event->text().isEmpty() && event->text().at(0).isPrint()) {
        mpEdit->setFocus(Qt::OtherFocusReason);
        QApplication::sendEvent(mpEdit, event);
    }
    // A key the box leaves, such as Shift on its own, would go on up to the map, which hands keys to the command line
    event->accept();
}

bool MapSearchPanel::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        // Or a tooltip none of the panel's widgets has would be the map's, for whatever lies under the panel
        QFrame::event(event);
        event->accept();
        return true;
    }
    return QFrame::event(event);
}

void MapSearchPanel::dismiss()
{
    hide();
    emit closed();
}

void MapSearchPanel::escape()
{
    if (mpEdit->text().isEmpty()) {
        dismiss();
    } else {
        mpEdit->clear();
    }
}

void MapSearchPanel::step(const int rows)
{
    const int count = mpList->count();
    if (!count) {
        return;
    }
    const int row = mpList->currentRow();
    const int next = row < 0 ? (rows > 0 ? 0 : count - 1) : ((row + rows) % count + count) % count;
    mpList->setCurrentRow(next);
    mpList->scrollToItem(mpList->item(next));
}

bool MapSearchPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress) {
        return QFrame::eventFilter(watched, event);
    }
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->matches(QKeySequence::Find)) {
        mpEdit->setFocus(Qt::ShortcutFocusReason);
        mpEdit->selectAll();
        return true;
    }
    switch (keyEvent->key()) {
    case Qt::Key_Escape:
        escape();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (mpList->currentRow() >= 0) {
            emit resultChosen(mpList->currentRow(), keyEvent->modifiers().testFlag(Qt::ControlModifier));
        }
        return true;
    case Qt::Key_Up:
        step(-1);
        return true;
    case Qt::Key_Down:
        step(1);
        return true;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        if (watched == mpEdit) {
            QApplication::sendEvent(mpList, event);
            return true;
        }
        break;
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}
