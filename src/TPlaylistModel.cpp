// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "TPlaylistModel.h"
#include "TMediaPlaylist.h"

#include <QFileInfo>
#include <QUrl>

TPlaylistModel::TPlaylistModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_playlist.reset(new TMediaPlaylist);
    connect(m_playlist.data(), &TMediaPlaylist::mediaAboutToBeInserted, this,
            &TPlaylistModel::beginInsertItems);
    connect(m_playlist.data(), &TMediaPlaylist::mediaInserted, this,
            &TPlaylistModel::endInsertItems);
    connect(m_playlist.data(), &TMediaPlaylist::mediaAboutToBeRemoved, this,
            &TPlaylistModel::beginRemoveItems);
    connect(m_playlist.data(), &TMediaPlaylist::mediaRemoved, this, &TPlaylistModel::endRemoveItems);
    connect(m_playlist.data(), &TMediaPlaylist::mediaChanged, this, &TPlaylistModel::changeItems);
}

TPlaylistModel::~TPlaylistModel() = default;

int TPlaylistModel::rowCount(const QModelIndex &parent) const
{
    return m_playlist && !parent.isValid() ? m_playlist->mediaCount() : 0;
}

int TPlaylistModel::columnCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? ColumnCount : 0;
}

QModelIndex TPlaylistModel::index(int row, int column, const QModelIndex &parent) const
{
    return m_playlist && !parent.isValid() && row >= 0 && row < m_playlist->mediaCount()
                    && column >= 0 && column < ColumnCount
            ? createIndex(row, column)
            : QModelIndex();
}

QModelIndex TPlaylistModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);

    return QModelIndex();
}

QVariant TPlaylistModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::DisplayRole) {
        QVariant value = m_data[index];
        if (!value.isValid() && index.column() == Title) {
            QUrl location = m_playlist->media(index.row());
            return QFileInfo(location.path()).fileName();
        }

        return value;
    }
    return QVariant();
}

TMediaPlaylist *TPlaylistModel::playlist() const
{
    return m_playlist.data();
}

bool TPlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    m_data[index] = value;
    emit dataChanged(index, index);
    return true;
}

void TPlaylistModel::beginInsertItems(int start, int end)
{
    m_data.clear();
    beginInsertRows(QModelIndex(), start, end);
}

void TPlaylistModel::endInsertItems()
{
    endInsertRows();
}

void TPlaylistModel::beginRemoveItems(int start, int end)
{
    m_data.clear();
    beginRemoveRows(QModelIndex(), start, end);
}

void TPlaylistModel::endRemoveItems()
{
    endInsertRows();
}

void TPlaylistModel::changeItems(int start, int end)
{
    m_data.clear();
    emit dataChanged(index(start, 0), index(end, ColumnCount));
}