#ifndef TTABBAR_H
#define TTABBAR_H

/***************************************************************************
 *   Copyright (C) 2018, 2020-2022 by Stephen Lyons                        *
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

#include <QColor>
#include <QHash>
#include <QProxyStyle>
#include <QRect>
#include <QSet>
#include <QString>
#include <QTabBar>

class QStyleOptionTab;

namespace uiDesign {
struct ThemeTokens;
}

// Connection status indicator drawn next to the tab text by TStyle. We paint
// it ourselves rather than via QTabBar::setTabIcon() because the macOS native
// style adds substantial padding around any tab icon - see issue #9213.
enum class TabConnectionIndicator {
    None,
    Connected,
    Connecting,
    Disconnected,
    Error,
};

// Painted, not styled: a QTabBar::tab rule hands the tab to QStyleSheetStyle, which has no
// per-tab state for the font emphasis or the indicator; QTabBar's drag offsets are private
class TStyle : public QProxyStyle
{
public:
    static constexpr int sIndicatorDiameter = 8;
    static constexpr int sIndicatorGap = 6;
    static constexpr int sIndicatorReservedWidth = sIndicatorDiameter + sIndicatorGap;
    static constexpr int sIndicatorRingWidth = 2;
    // accentText at the 4.5:1 floor fades into the wash on the grey page; 7:1 matches the sidebar's chosen row
    static constexpr qreal sChosenWordMinimumRatio = 7.0;

    explicit TStyle(QTabBar* bar)
    : mpTabBar(bar)
    {
    }

    ~TStyle() = default;

    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override;
    void setTabBold(const QString& tabName, const bool state) { setNamedTabState(tabName, state, mBoldTabsSet); }
    void setTabBold(const int index, const bool state) { setIndexedTabState(index, state, mBoldTabsSet); }
    void setTabItalic(const QString& tabName, const bool state) { setNamedTabState(tabName, state, mItalicTabsSet); }
    void setTabItalic(const int index, const bool state) { setIndexedTabState(index, state, mItalicTabsSet); }
    void setTabUnderline(const QString& tabName, const bool state) { setNamedTabState(tabName, state, mUnderlineTabsSet); }
    void setTabUnderline(const int index, const bool state) { setIndexedTabState(index, state, mUnderlineTabsSet); }
    bool tabBold(const QString& tabName) const { return namedTabState(tabName, mBoldTabsSet); }
    bool tabBold(const int index) const { return indexedTabState(index, mBoldTabsSet); }
    bool tabItalic(const QString& tabName) const { return namedTabState(tabName, mItalicTabsSet); }
    bool tabItalic(const int index) const { return indexedTabState(index, mItalicTabsSet); }
    bool tabUnderline(const QString& tabName) const { return namedTabState(tabName, mUnderlineTabsSet); }
    bool tabUnderline(const int index) const { return indexedTabState(index, mUnderlineTabsSet); }
    // Returns true only if the stored state actually changed, so callers
    // can skip unnecessary repaints.
    bool setTabConnectionIndicator(const QString& tabName, TabConnectionIndicator state);
    bool setTabConnectionIndicator(int index, TabConnectionIndicator state);
    TabConnectionIndicator tabConnectionIndicator(const QString& tabName) const;
    TabConnectionIndicator tabConnectionIndicator(int index) const;

private:
    bool scriptSheetTakesTheTabs() const;
    // indicator and close stay null on a tab that carries neither
    struct ChipLayout
    {
        QRect chip;
        QRect indicator;
        QRect text;
        QRect close;
    };

    // surface is the fill as composited over the page: what anything on the chip is read against
    struct ChipFace
    {
        QColor fill;
        QColor word;
        QColor surface;
        bool bar = false;
        bool outline = false;
    };

    // QTabBar::tabSizeHint()'s "padding += 4" per tab button; the word leaves it before
    // the cross so both sit where Qt measured them
    static constexpr int sCloseButtonGap = 4;

    ChipFace chipFace(const QStyleOptionTab* tabOption, const uiDesign::ThemeTokens& tokens) const;
    ChipLayout layoutChip(const QStyleOptionTab* tabOption) const;
    int tabIndexOf(const QStyleOptionTab* tabOption) const;
    bool indexedTabState(int index, const QSet<QString>& effect) const;
    bool namedTabState(const QString& tabName, const QSet<QString>& effect) const;
    void setNamedTabState(const QString& tabName, bool state, QSet<QString>& effect);
    void setIndexedTabState(int index, bool state, QSet<QString>& effect);
    void paintChip(QPainter* painter, const QStyleOptionTab* tabOption, const QWidget* widget, const bool shape, const bool label) const;
    void paintThroughApplicationSheet(ControlElement element, const QStyleOptionTab* tabOption, QPainter* painter, const QWidget* widget) const;
    void paintConnectionIndicator(QPainter* painter, const QRect& box, TabConnectionIndicator state, const uiDesign::ThemeTokens& tokens, const QColor& surface, const QColor& ringInk) const;

    QTabBar* mpTabBar;
    // The sets that hold the tab names that have the particular effect, we
    // use the text rather than the indexes because the tabs could be capable of
    // being reordered, but the names are expected to be constant (or if the
    // "name" changes then code will be put in place to handle that)!
    // One of these is to be used as the argument to the four private methods.
    QSet<QString> mBoldTabsSet;
    QSet<QString> mItalicTabsSet;
    QSet<QString> mUnderlineTabsSet;
    // Keyed by tab name (not index) for the same reorder-tolerance reason.
    QHash<QString, TabConnectionIndicator> mConnectionIndicators;
};

class TTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TTabBar(QWidget* parent)
    : QTabBar(parent)
    , mStyle(this)
    {
        setStyle(&mStyle);
        setAcceptDrops(true);
        // Without it QTabBar::initStyleOption() never sets State_MouseOver
        setAttribute(Qt::WA_Hover);
        setDrawBase(false);
    }
    ~TTabBar() = default;

    QSize tabSizeHint(int index) const override;
    void applyPrefixToDisplayedText(const int index, const QString& prefix = QString());
    void applyPrefixToDisplayedText(const QString& tabName, const QString& prefix = QString());
    void setTabBold(const QString& tabName, const bool state) { mStyle.setTabBold(tabName, state); }
    void setTabBold(const int index, const bool state) { mStyle.setTabBold(index, state); }
    void setTabItalic(const QString& tabName, const bool state) { mStyle.setTabItalic(tabName, state); }
    void setTabItalic(const int index, const bool state) { mStyle.setTabItalic(index, state); }
    void setTabUnderline(const QString& tabName, const bool state) { mStyle.setTabUnderline(tabName, state); }
    void setTabUnderline(const int index, const bool state) { mStyle.setTabUnderline(index, state); }
    bool tabBold(const QString& tabName) const { return mStyle.tabBold(tabName); }
    bool tabBold(const int index) const { return mStyle.tabBold(index); }
    bool tabItalic(const QString& tabName) const { return mStyle.tabItalic(tabName); }
    bool tabItalic(const int index) const { return mStyle.tabItalic(index); }
    bool tabUnderline(const QString& tabName) const { return mStyle.tabUnderline(tabName); }
    bool tabUnderline(const int index) const { return mStyle.tabUnderline(index); }
    void setTabConnectionIndicator(const QString& tabName, TabConnectionIndicator state)
    {
        if (mStyle.setTabConnectionIndicator(tabName, state)) {
            // updateGeometry() invalidates the cached tab sizes so tabSizeHint()
            // is consulted again. For the brief Connecting stage we paint
            // synchronously so a fast follow-up Connected transition cannot
            // coalesce the pending paint and skip the yellow dot entirely;
            // other transitions can take the cheaper deferred update().
            updateGeometry();
            if (state == TabConnectionIndicator::Connecting) {
                repaint();
            } else {
                update();
            }
        }
    }
    void setTabConnectionIndicator(const int index, TabConnectionIndicator state)
    {
        if (mStyle.setTabConnectionIndicator(index, state)) {
            updateGeometry();
            if (state == TabConnectionIndicator::Connecting) {
                repaint();
            } else {
                update();
            }
        }
    }
    TabConnectionIndicator tabConnectionIndicator(const QString& tabName) const { return mStyle.tabConnectionIndicator(tabName); }
    TabConnectionIndicator tabConnectionIndicator(const int index) const { return mStyle.tabConnectionIndicator(index); }
    QString tabName(const int index) const;
    int tabIndex(const QString& tabName) const;
    void removeTab(const QString& tabName);
    void removeTab(int);
    void markActivity(const QString& tabName, bool isLowerPriorityChange);
    void clearActivity(int index);
    QStringList tabNames() const;
    void refreshAfterApplicationStyleChange();

signals:
    void tabDetachRequested(int index, const QPoint& globalPos);
    void tabReattachRequested(const QString& tabName, int index);

private:
    void relayoutTab(int index);
    // True only when the button still had the application style, the one case needing a re-measure
    bool adoptCloseButton(int index);

    // This instance of TStyle needs a pointer to a QTabBar on instantiation:
    TStyle mStyle;

    // Drag and drop functionality
    QPoint mDragStartPos;
    // The profile, not its index: Qt reorders the bar while the tab is dragged,
    // and a profile can open or close before the drag ends
    QString mDragTabName;
    bool mDetachEnabled = true;
    static const int DETACH_DISTANCE_THRESHOLD = 80;
    qint64 mDragStartTime = 0;
    bool mPendingDetach = false;

private slots:
    void onDetachedTabReattach(const QString& tabName);

protected:
    void tabInserted(int index) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};

#endif // TTABBAR_H
