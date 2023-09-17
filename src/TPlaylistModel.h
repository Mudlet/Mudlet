// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TPLAYLISTMODEL_H
#define TPLAYLISTMODEL_H

#include <QAbstractItemModel>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
class TMediaPlaylist;
QT_END_NAMESPACE

class TPlaylistModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column { Title = 0, ColumnCount };

    explicit TPlaylistModel(QObject *parent = nullptr);
    ~TPlaylistModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    TMediaPlaylist *playlist() const;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::DisplayRole) override;

private slots:
    void beginInsertItems(int start, int end);
    void endInsertItems();
    void beginRemoveItems(int start, int end);
    void endRemoveItems();
    void changeItems(int start, int end);

private:
    QScopedPointer<TMediaPlaylist> m_playlist;
    QMap<QModelIndex, QVariant> m_data;
};

#endif // TPLAYLISTMODEL_H