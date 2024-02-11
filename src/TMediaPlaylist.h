// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TMEDIAPLAYLIST_H
#define TMEDIAPLAYLIST_H

#include <QObject>

#include <qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

class TMediaPlaylistPrivate;
class TMediaPlaylist : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TMediaPlaylist::PlaybackMode playbackMode READ playbackMode WRITE setPlaybackMode
                       NOTIFY playbackModeChanged)
    Q_PROPERTY(QUrl currentMedia READ currentMedia NOTIFY currentMediaChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    enum PlaybackMode { CurrentItemOnce, CurrentItemInLoop, Sequential, Loop };
    Q_ENUM(PlaybackMode)
    enum Error { NoError, FormatError, FormatNotSupportedError, NetworkError, AccessDeniedError };
    Q_ENUM(Error)

    explicit TMediaPlaylist(QObject *parent = nullptr);
    virtual ~TMediaPlaylist();

    PlaybackMode playbackMode() const;
    void setPlaybackMode(PlaybackMode mode);

    int currentIndex() const;
    QUrl currentMedia() const;

    int nextIndex(int steps = 1) const;
    int previousIndex(int steps = 1) const;

    QUrl media(int index) const;

    int mediaCount() const;
    bool isEmpty() const;

    void addMedia(const QUrl &content);
    void addMedia(const QList<QUrl> &items);
    bool insertMedia(int index, const QUrl &content);
    bool insertMedia(int index, const QList<QUrl> &items);
    bool moveMedia(int from, int to);
    bool removeMedia(int pos);
    bool removeMedia(int start, int end);
    void clear();

    void load(const QUrl &location, const char *format = nullptr);
    void load(QIODevice *device, const char *format = nullptr);

    bool save(const QUrl &location, const char *format = nullptr) const;
    bool save(QIODevice *device, const char *format) const;

    Error error() const;
    QString errorString() const;

public slots:
    void shuffle();

    void next();
    void previous();

    void setCurrentIndex(int index);

signals:
    void currentIndexChanged(int index);
    void playbackModeChanged(TMediaPlaylist::PlaybackMode mode);
    void currentMediaChanged(const QUrl &);

    void mediaAboutToBeInserted(int start, int end);
    void mediaInserted(int start, int end);
    void mediaAboutToBeRemoved(int start, int end);
    void mediaRemoved(int start, int end);
    void mediaChanged(int start, int end);

    void loaded();
    void loadFailed();

private:
    TMediaPlaylistPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TMediaPlaylist)
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(TMediaPlaylist, PlaybackMode)
Q_MEDIA_ENUM_DEBUG(TMediaPlaylist, Error)

#endif // TMEDIAPLAYLIST_H