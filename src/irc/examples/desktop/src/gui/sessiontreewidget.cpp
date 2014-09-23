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

#include "sessiontreewidget.h"
#include "sessiontreedelegate.h"
#include "sessiontreeitem.h"
#include "sharedtimer.h"
#include "session.h"
#include <QContextMenuEvent>
#include <QTimer>

SessionTreeWidget::SessionTreeWidget(QWidget* parent) : QTreeWidget(parent)
{
    setAnimated(true);
    setIndentation(0);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setFrameStyle(QFrame::NoFrame);
    setItemDelegate(new SessionTreeDelegate(this));

    d.colors[Active] = palette().color(QPalette::WindowText);
    d.colors[Inactive] = palette().color(QPalette::Disabled, QPalette::Highlight);
    d.colors[Alert] = palette().color(QPalette::Highlight);
    d.colors[Highlight] = palette().color(QPalette::Highlight);
    d.alertColor = d.colors.value(Alert);

    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(onItemCollapsed(QTreeWidgetItem*)));

    d.prevShortcut = new QShortcut(this);
    connect(d.prevShortcut, SIGNAL(activated()), this, SLOT(moveToPrevItem()));

    d.nextShortcut = new QShortcut(this);
    connect(d.nextShortcut, SIGNAL(activated()), this, SLOT(moveToNextItem()));

    d.prevUnreadShortcut = new QShortcut(this);
    connect(d.prevUnreadShortcut, SIGNAL(activated()), this, SLOT(moveToPrevUnreadItem()));

    d.nextUnreadShortcut = new QShortcut(this);
    connect(d.nextUnreadShortcut, SIGNAL(activated()), this, SLOT(moveToNextUnreadItem()));

    d.expandShortcut = new QShortcut(this);
    connect(d.expandShortcut, SIGNAL(activated()), this, SLOT(expandCurrentSession()));

    d.collapseShortcut = new QShortcut(this);
    connect(d.collapseShortcut, SIGNAL(activated()), this, SLOT(collapseCurrentSession()));

    applySettings(d.settings);
}

QSize SessionTreeWidget::sizeHint() const
{
    return QSize(20 * fontMetrics().width('#'), QTreeWidget::sizeHint().height());
}

QColor SessionTreeWidget::statusColor(SessionTreeWidget::ItemStatus status) const
{
    return d.colors.value(status);
}

void SessionTreeWidget::setStatusColor(SessionTreeWidget::ItemStatus status, const QColor& color)
{
    d.colors[status] = color;
}

QColor SessionTreeWidget::currentAlertColor() const
{
    return d.alertColor;
}

QList<Session*> SessionTreeWidget::sessions() const
{
    return d.sessions.keys();
}

SessionTreeItem* SessionTreeWidget::sessionItem(Session* session) const
{
    return d.sessions.value(session);
}

void SessionTreeWidget::addSession(Session* session)
{
    SessionTreeItem* item = new SessionTreeItem(session, this);
    d.sessions.insert(session, item);

    connect(session, SIGNAL(activeChanged(bool)), this, SLOT(updateSession()));
    connect(session, SIGNAL(nameChanged(QString)), this, SLOT(updateSession()));
    connect(session, SIGNAL(networkChanged(QString)), this, SLOT(updateSession()));
    updateSession(session);
}

void SessionTreeWidget::removeSession(Session *session)
{
    delete d.sessions.take(session);
}

void SessionTreeWidget::addView(Session* session, const QString& view)
{
    SessionTreeItem* parent = d.sessions.value(session);
    if (parent)
    {
        SessionTreeItem* item = new SessionTreeItem(session, parent);
        item->setText(0, view);
    }
}

void SessionTreeWidget::removeView(Session* session, const QString& view)
{
    SessionTreeItem* parent = d.sessions.value(session);
    if (parent)
        delete parent->findChild(view);
}

void SessionTreeWidget::renameView(Session* session, const QString& from, const QString& to)
{
    SessionTreeItem* parent = d.sessions.value(session);
    if (parent)
    {
        SessionTreeItem* item = parent->findChild(from);
        if (item)
            item->setText(0, to);
    }
}

void SessionTreeWidget::setCurrentView(Session* session, const QString& view)
{
    SessionTreeItem* item = d.sessions.value(session);
    if (item && !view.isEmpty())
        item = item->findChild(view);
    setCurrentItem(item);
}

void SessionTreeWidget::moveToNextItem()
{
    QTreeWidgetItem* item = nextItem(currentItem());
    if (!item)
        item = topLevelItem(0);
    setCurrentItem(item);
}

void SessionTreeWidget::moveToPrevItem()
{
    QTreeWidgetItem* item = previousItem(currentItem());
    if (!item)
        item = previousItem(lastItem());
    setCurrentItem(item);
}

void SessionTreeWidget::moveToNextUnreadItem()
{
    QTreeWidgetItem* current = currentItem();
    if (current)
    {
        QTreeWidgetItemIterator it(current);
        while (*++it && *it != current)
        {
            SessionTreeItem* item = static_cast<SessionTreeItem*>(*it);
            if (item->isAlerted() || item->isHighlighted())
            {
                setCurrentItem(item);
                break;
            }
        }
    }
}

void SessionTreeWidget::moveToPrevUnreadItem()
{
    QTreeWidgetItem* current = currentItem();
    if (current)
    {
        QTreeWidgetItemIterator it(current);
        while (*--it && *it != current)
        {
            SessionTreeItem* item = static_cast<SessionTreeItem*>(*it);
            if (item->isAlerted() || item->isHighlighted())
            {
                setCurrentItem(item);
                break;
            }
        }
    }
}

void SessionTreeWidget::expandCurrentSession()
{
    QTreeWidgetItem* item = currentItem();
    if (item && item->parent())
        item = item->parent();
    if (item)
        expandItem(item);
}

void SessionTreeWidget::collapseCurrentSession()
{
    QTreeWidgetItem* item = currentItem();
    if (item && item->parent())
        item = item->parent();
    if (item)
    {
        collapseItem(item);
        setCurrentItem(item);
    }
}

void SessionTreeWidget::alert(SessionTreeItem* item)
{
    if (d.alertedItems.isEmpty())
        SharedTimer::instance()->registerReceiver(this, "alertTimeout");
    d.alertedItems.insert(item);
}

void SessionTreeWidget::unalert(SessionTreeItem* item)
{
    if (d.alertedItems.remove(item) && d.alertedItems.isEmpty())
        SharedTimer::instance()->unregisterReceiver(this, "alertTimeout");
}

void SessionTreeWidget::applySettings(const Settings& settings)
{
    QColor color(settings.colors.value(Settings::Highlight));
    setStatusColor(Alert, color);
    setStatusColor(Highlight, color);

    d.prevShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::NavigateUp)));
    d.nextShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::NavigateDown)));
    d.prevUnreadShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::NextUnreadUp)));
    d.nextUnreadShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::NextUnreadDown)));
    d.expandShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::NavigateRight)));
    d.collapseShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::NavigateLeft)));
    d.settings = settings;
}

void SessionTreeWidget::contextMenuEvent(QContextMenuEvent* event)
{
    SessionTreeItem* item = static_cast<SessionTreeItem*>(itemAt(event->pos()));
    if (item)
        emit menuRequested(item, event->globalPos());
}

bool SessionTreeWidget::event(QEvent* event)
{
    if (event->type() == QEvent::WindowActivate)
        delayedItemReset();
    return QTreeWidget::event(event);
}

void SessionTreeWidget::updateSession(Session* session)
{
    if (!session)
        session = qobject_cast<Session*>(sender());
    SessionTreeItem* item = d.sessions.value(session);
    if (item)
    {
        item->setText(0, session->name().isEmpty() ? session->host() : session->name());
        item->setInactive(!session->isActive());
    }
}

void SessionTreeWidget::onItemSelectionChanged()
{
    SessionTreeItem* item = static_cast<SessionTreeItem*>(selectedItems().value(0));
    if (item)
    {
        resetItem(item);
        emit currentViewChanged(item->session(), item->parent() ? item->text(0) : QString());
    }
}

void SessionTreeWidget::onItemExpanded(QTreeWidgetItem* item)
{
    static_cast<SessionTreeItem*>(item)->emitDataChanged();
}

void SessionTreeWidget::onItemCollapsed(QTreeWidgetItem* item)
{
    static_cast<SessionTreeItem*>(item)->emitDataChanged();
}

void SessionTreeWidget::delayedItemReset()
{
    SessionTreeItem* item = static_cast<SessionTreeItem*>(currentItem());
    if (item)
    {
        d.resetedItems.insert(item);
        QTimer::singleShot(500, this, SLOT(delayedItemResetTimeout()));
    }
}

void SessionTreeWidget::delayedItemResetTimeout()
{
    if (!d.resetedItems.isEmpty())
    {
        foreach (SessionTreeItem* item, d.resetedItems)
            resetItem(item);
        d.resetedItems.clear();
    }
}

void SessionTreeWidget::alertTimeout()
{
    bool active = d.alertColor == d.colors.value(Active);
    d.alertColor = d.colors.value(active ? Alert : Active);

    foreach (SessionTreeItem* item, d.alertedItems)
    {
        item->emitDataChanged();
        if (SessionTreeItem* p = static_cast<SessionTreeItem*>(item->parent()))
            if (!p->isExpanded())
                p->emitDataChanged();
    }
}

void SessionTreeWidget::resetItem(SessionTreeItem* item)
{
    unalert(item);
    item->setAlerted(false);
    item->setHighlighted(false);
}

QTreeWidgetItem* SessionTreeWidget::lastItem() const
{
    QTreeWidgetItem* item = topLevelItem(topLevelItemCount() - 1);
    if (item->childCount() > 0)
        item = item->child(item->childCount() - 1);
    return item;
}

QTreeWidgetItem* SessionTreeWidget::nextItem(QTreeWidgetItem* from) const
{
    if (!from)
        return 0;
    QTreeWidgetItemIterator it(from);
    while (*++it)
    {
        if (!(*it)->parent() || (*it)->parent()->isExpanded())
            break;
    }
    return *it;
}

QTreeWidgetItem* SessionTreeWidget::previousItem(QTreeWidgetItem* from) const
{
    if (!from)
        return 0;
    QTreeWidgetItemIterator it(from);
    while (*--it)
    {
        if (!(*it)->parent() || (*it)->parent()->isExpanded())
            break;
    }
    return *it;
}
