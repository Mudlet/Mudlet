#ifndef MUDLET_TOOLBAROVERFLOW_H
#define MUDLET_TOOLBAROVERFLOW_H

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

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QAction;
class QToolBar;
class QToolButton;

namespace uiDesign {

struct ThemeTokens;

// The bar's answer to QStyle::PM_ToolBarExtensionExtent, which MudletProxyStyle returns
inline constexpr char scmProp_toolBarOverflowExtent[] = "uiToolBarOverflowExtent";
// On Qt's own overflow button once this class draws its glyph, so a style polish leaves the glyph alone
inline constexpr char scmProp_toolBarOverflowOwned[] = "uiToolBarOverflowOwned";

// Makes Qt's overflow button a button of the bar: a chevron and "3 more", where Qt leaves an unlabelled sliver
class ToolBarOverflow : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ToolBarOverflow)
    // Call from every restyle of the bar's window; the first call for a bar makes its ToolBarOverflow
    static void restyle(QToolBar* pBar, const ThemeTokens& tokens);
    [[nodiscard]] static QString word(const int hiddenCount);

    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    ToolBarOverflow(QToolBar* pBar, QToolButton* pButton);

    void applyGlyph(const ThemeTokens& tokens);
    void queueRefresh();
    void refresh();
    void fitToBar();
    void widenToTheWord();
    void showHiddenCount();
    void styleItsMenu();
    [[nodiscard]] QList<QAction*> countedActions() const;

    QPointer<QToolBar> mpBar;
    QPointer<QToolButton> mpButton;
    QString mFitFor;
    int mWordWidthDownAnEdge = 0;
    bool mRefreshQueued = false;
    bool mGlyphStale = false;
};

} // namespace uiDesign

#endif // MUDLET_TOOLBAROVERFLOW_H
