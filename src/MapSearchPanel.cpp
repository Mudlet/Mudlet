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

#include <QCheckBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

MapSearchPanel::MapSearchPanel(QWidget* parent)
: QFrame(parent)
{
    setObjectName(qsl("mapSearchPanel"));
    setFrameShape(QFrame::StyledPanel);
    setAutoFillBackground(true);
    // Or a click, a drag or the wheel the panel's own widgets leave would reach the map under it
    setAttribute(Qt::WA_NoMousePropagation);
    // Painted by the style sheet: over the 3D view a filled palette background is not drawn, leaving the
    // checkbox's text on the map
    setStyleSheet(qsl("QFrame#mapSearchPanel { background-color: palette(window); border: 1px solid palette(mid); border-radius: 6px; }"));
    mpEdit = new QLineEdit(this);
    //: Placeholder of the search box over the 3D map
    mpEdit->setPlaceholderText(tr("Find a room, area or label"));
    //: Tooltip of the search box over the 3D map
    mpEdit->setToolTip(tr("<p>Searches every area of the map for room names, area names, room numbers and map labels.</p>"
                          "<p>Enter flies to the place chosen, Ctrl+Enter plans a route to a room or area, Escape closes the search.</p>"));
    mpEdit->setClearButtonEnabled(true);
    //: Checkbox by the search box over the 3D map: whether map labels are searched too
    mpLabels = new QCheckBox(tr("Labels"), this);
    mpLabels->setChecked(true);
    auto* closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setText(qsl("✕"));
    //: Tooltip of the button that closes the search box over the 3D map
    closeButton->setToolTip(tr("Close the search"));
    mpList = new QListWidget(this);
    mpList->setContextMenuPolicy(Qt::CustomContextMenu);
    mpList->setUniformItemSizes(true);
    mpList->hide();
    mpSummary = new QLabel(this);
    mpSummary->hide();

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(mpEdit, 1);
    row->addWidget(mpLabels);
    row->addWidget(closeButton);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    layout->addLayout(row);
    layout->addWidget(mpList, 1);
    layout->addWidget(mpSummary);

    connect(mpEdit, &QLineEdit::textChanged, this, &MapSearchPanel::queryChanged);
    connect(mpLabels, &QCheckBox::toggled, this, &MapSearchPanel::queryChanged);
    connect(closeButton, &QToolButton::clicked, this, &MapSearchPanel::dismiss);
    connect(mpList, &QListWidget::itemActivated, this, [this]() {
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

void MapSearchPanel::setResults(const QStringList& lines, const QString& summary)
{
    mpList->clear();
    mpList->addItems(lines);
    mpList->setCurrentRow(lines.isEmpty() ? -1 : 0);
    constexpr qsizetype maxRowsShown = 12;
    const int rowsShown = static_cast<int>(std::min(lines.size(), maxRowsShown));
    // At most, so that a short view can squeeze it
    mpList->setMaximumHeight(rowsShown * mpList->sizeHintForRow(0) + 2 * mpList->frameWidth());
    mpList->setVisible(!lines.isEmpty());
    mpSummary->setText(summary);
    mpSummary->setVisible(!summary.isEmpty());
}

void MapSearchPanel::keyPressEvent(QKeyEvent* event)
{
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

bool MapSearchPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress) {
        return QFrame::eventFilter(watched, event);
    }
    auto* keyEvent = static_cast<QKeyEvent*>(event);
    switch (keyEvent->key()) {
    case Qt::Key_Escape:
        dismiss();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (mpList->currentRow() >= 0) {
            emit resultChosen(mpList->currentRow(), keyEvent->modifiers().testFlag(Qt::ControlModifier));
        }
        return true;
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        if (watched == mpEdit) {
            QCoreApplication::sendEvent(mpList, event);
            return true;
        }
        break;
    default:
        break;
    }
    return QFrame::eventFilter(watched, event);
}
