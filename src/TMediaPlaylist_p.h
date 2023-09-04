// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TMEDIAPLAYLIST_P_H
#define TMEDIAPLAYLIST_P_H

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
#include "TPlaylistFileParser.h"

#include <QUrl>

#include <QDebug>

#ifdef Q_MOC_RUN
#    pragma Q_MOC_EXPAND_MACROS
#endif

QT_BEGIN_NAMESPACE

//class TMediaPlaylistControl;

class TMediaPlaylistPrivate
{
    Q_DECLARE_PUBLIC(TMediaPlaylist)
public:
    TMediaPlaylistPrivate();

    virtual ~TMediaPlaylistPrivate();

    void loadFailed(TMediaPlaylist::Error error, const QString &errorString);

    void loadFinished();

    bool checkFormat(const char *format) const;

    void ensureParser();

    int nextPosition(int steps) const;
    int prevPosition(int steps) const;

    QList<QUrl> playlist;

    int currentPos = -1;
    TMediaPlaylist::PlaybackMode playbackMode = TMediaPlaylist::Sequential;

    TPlaylistFileParser *parser = nullptr;
    mutable TMediaPlaylist::Error error;
    mutable QString errorString;

    TMediaPlaylist *q_ptr;
};

QT_END_NAMESPACE

#endif // TMEDIAPLAYLIST_P_H