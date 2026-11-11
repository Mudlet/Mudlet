#ifndef MUDLET_EDITORTREEROWMETRICS_H
#define MUDLET_EDITORTREEROWMETRICS_H

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

#include <QtCore/qnamespace.h>

#include <algorithm>

class QColor;
class QModelIndex;
class QPixmap;

// Shared by EditorTreeDelegate and VariableTreeDelegate so a row in any of the seven trees measures alike
namespace uiDesign {

inline constexpr int scmTreeDotDiameter = 9;
inline constexpr int scmTreeDotGap = 6;
inline constexpr qreal scmTreeHollowPenWidth = 1.5;
inline constexpr qreal scmTreeDotCentreDiameter = 3.0;
// A 9px target is not one to ask for accuracy on
inline constexpr int scmTreeMarkHitSlack = 2;
// The dot's square, so the chevron and the dot read as one row of marks
inline constexpr int scmTreeChevronBox = 9;
inline constexpr qreal scmTreeChevronArm = 2.6;
inline constexpr qreal scmTreeChevronPenWidth = 1.4;
// Matches scmGlyphSize in SearchResultDelegate.cpp, the panel's other list
inline constexpr int scmTreeMarkSize = 16;
// Left whether or not a row has a mark, so every row of a tree is one height
inline constexpr int scmTreeSlotHeight = std::max({scmTreeMarkSize, scmTreeDotDiameter, scmTreeChevronBox});
inline constexpr int scmTreeFallbackIndentStep = 20;
// The decoration key holds the depth, so a deep enough tree would otherwise grow the cache a level at a time
inline constexpr int scmTreeDecorationCacheLimit = 256;
inline constexpr Qt::KeyboardModifiers scmTreeSelectionModifiers = Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier;

// QTreeView counts this to indent a row but does not expose it
int treeRowLevelOf(const QModelIndex& index);

// Drawn rather than loaded from a file: two strokes, in whatever ink the tree's chrome is currently in
QPixmap treeRowChevronGlyph(const bool open, const QColor& ink, const qreal ratio);

// Each form differs in shape as well as ink, for a reader who cannot separate the inks
enum class TreeDotForm { Filled, Ring, RingWithCentre };

QPixmap treeRowDotGlyph(const TreeDotForm form, const QColor& ink, const qreal ratio);

} // namespace uiDesign

#endif // MUDLET_EDITORTREEROWMETRICS_H
