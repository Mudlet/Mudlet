/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFILESYSTEMMODEL_H
#define QFILESYSTEMMODEL_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qpair.h>
#include <QtCore/qdir.h>
#include <QtGui/qicon.h>
#include <QtCore/qdiriterator.h>

QT_REQUIRE_CONFIG(filesystemmodel);

QT_BEGIN_NAMESPACE

class ExtendedInformation;
class QFileSystemModelPrivate;
class QFileIconProvider;

class Q_WIDGETS_EXPORT QFileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool resolveSymlinks READ resolveSymlinks WRITE setResolveSymlinks)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool nameFilterDisables READ nameFilterDisables WRITE setNameFilterDisables)
    Q_PROPERTY(Options options READ options WRITE setOptions)

Q_SIGNALS:
    void rootPathChanged(const QString &newPath);
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void directoryLoaded(const QString &path);

public:
    enum Roles {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FilePermissions = Qt::UserRole + 3
    };

    enum Option
    {
        DontWatchForChanges         = 0x00000001,
        DontResolveSymlinks         = 0x00000002,
        DontUseCustomDirectoryIcons = 0x00000004
    };
    Q_ENUM(Option)
    Q_DECLARE_FLAGS(Options, Option)

    explicit QFileSystemModel(QObject *parent = nullptr);
    ~QFileSystemModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(const QString &path, int column = 0) const;
    QModelIndex parent(const QModelIndex &child) const override;
    using QObject::parent;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant myComputer(int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;

    // QFileSystemModel specific API
    QModelIndex setRootPath(const QString &path);
    QString rootPath() const;
    QDir rootDirectory() const;

    void setIconProvider(QFileIconProvider *provider);
    QFileIconProvider *iconProvider() const;

    void setFilter(QDir::Filters filters);
    QDir::Filters filter() const;

    void setResolveSymlinks(bool enable);
    bool resolveSymlinks() const;

    void setReadOnly(bool enable);
    bool isReadOnly() const;

    void setNameFilterDisables(bool enable);
    bool nameFilterDisables() const;

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setOption(Option option, bool on = true);
    bool testOption(Option option) const;
    void setOptions(Options options);
    Options options() const;

    QString filePath(const QModelIndex &index) const;
    bool isDir(const QModelIndex &index) const;
    qint64 size(const QModelIndex &index) const;
    QString type(const QModelIndex &index) const;
    QDateTime lastModified(const QModelIndex &index) const;

    QModelIndex mkdir(const QModelIndex &parent, const QString &name);
    bool rmdir(const QModelIndex &index);
    inline QString fileName(const QModelIndex &index) const;
    inline QIcon fileIcon(const QModelIndex &index) const;
    QFile::Permissions permissions(const QModelIndex &index) const;
    QFileInfo fileInfo(const QModelIndex &index) const;
    bool remove(const QModelIndex &index);

protected:
    QFileSystemModel(QFileSystemModelPrivate &, QObject *parent = nullptr);
    void timerEvent(QTimerEvent *event) override;
    bool event(QEvent *event) override;

private:
    Q_DECLARE_PRIVATE(QFileSystemModel)
    Q_DISABLE_COPY(QFileSystemModel)

    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &directory, const QStringList &list))
    Q_PRIVATE_SLOT(d_func(), void _q_performDelayedSort())
    Q_PRIVATE_SLOT(d_func(), void _q_fileSystemChanged(const QString &path, const QVector<QPair<QString, QFileInfo> > &))
    Q_PRIVATE_SLOT(d_func(), void _q_resolvedName(const QString &fileName, const QString &resolvedName))

    friend class QFileDialogPrivate;
};

inline QString QFileSystemModel::fileName(const QModelIndex &aindex) const
{ return aindex.data(Qt::DisplayRole).toString(); }
inline QIcon QFileSystemModel::fileIcon(const QModelIndex &aindex) const
{ return qvariant_cast<QIcon>(aindex.data(Qt::DecorationRole)); }

Q_DECLARE_OPERATORS_FOR_FLAGS(QFileSystemModel::Options)

QT_END_NAMESPACE

#endif // QFILESYSTEMMODEL_H
