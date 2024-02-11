// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TPLAYLISTFILEPARSER_P_H
#define TPLAYLISTFILEPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "TMediaPlaylist.h"
#include "qtmultimediaglobal.h"

#include <QObject>

QT_BEGIN_NAMESPACE

class QIODevice;
class QUrl;
class QNetworkRequest;

class TPlaylistFileParserPrivate;

class TPlaylistFileParser : public QObject
{
    Q_OBJECT
public:
    TPlaylistFileParser(QObject *parent = nullptr);
    ~TPlaylistFileParser();

    enum FileType {
        UNKNOWN,
        M3U,
        M3U8, // UTF-8 version of M3U
        PLS
    };

    void start(const QUrl &media, QIODevice *stream = nullptr, const QString &mimeType = QString());
    void start(const QUrl &request, const QString &mimeType = QString());
    void start(QIODevice *stream, const QString &mimeType = QString());
    void abort();

    QList<QUrl> playlist;

signals:
    void newItem(const QVariant &content);
    void finished();
    void error(TMediaPlaylist::Error err, const QString &errorMsg);

private slots:
    void handleData();
    void handleError();

private:
    static FileType findByMimeType(const QString &mime);
    static FileType findBySuffixType(const QString &suffix);
    static FileType findByDataHeader(const char *data, quint32 size);
    static FileType findPlaylistType(QIODevice *device, const QString &mime);
    static FileType findPlaylistType(const QString &suffix, const QString &mime,
                                     const char *data = nullptr, quint32 size = 0);

    Q_DISABLE_COPY(TPlaylistFileParser)
    Q_DECLARE_PRIVATE(TPlaylistFileParser)
    QScopedPointer<TPlaylistFileParserPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // TPLAYLISTFILEPARSER_P_H