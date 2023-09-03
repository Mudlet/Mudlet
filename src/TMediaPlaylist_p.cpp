// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "TMediaPlaylist_p.h"

QT_BEGIN_NAMESPACE

TMediaPlaylistPrivate::TMediaPlaylistPrivate() : error(TMediaPlaylist::NoError) { }

TMediaPlaylistPrivate::~TMediaPlaylistPrivate()
{
    delete parser;
}

void TMediaPlaylistPrivate::loadFailed(TMediaPlaylist::Error error, const QString &errorString)
{
    this->error = error;
    this->errorString = errorString;

    emit q_ptr->loadFailed();
}

void TMediaPlaylistPrivate::loadFinished()
{
    q_ptr->addMedia(parser->playlist);

    emit q_ptr->loaded();
}

bool TMediaPlaylistPrivate::checkFormat(const char *format) const
{
    QLatin1String f(format);
    TPlaylistFileParser::FileType type =
            format ? TPlaylistFileParser::UNKNOWN : TPlaylistFileParser::M3U8;
    if (format) {
        if (f == QLatin1String("m3u") || f == QLatin1String("text/uri-list")
            || f == QLatin1String("audio/x-mpegurl") || f == QLatin1String("audio/mpegurl"))
            type = TPlaylistFileParser::M3U;
        else if (f == QLatin1String("m3u8") || f == QLatin1String("application/x-mpegURL")
                 || f == QLatin1String("application/vnd.apple.mpegurl"))
            type = TPlaylistFileParser::M3U8;
    }

    if (type == TPlaylistFileParser::UNKNOWN || type == TPlaylistFileParser::PLS) {
        error = TMediaPlaylist::FormatNotSupportedError;
        errorString = TMediaPlaylist::tr("This file format is not supported.");
        return false;
    }
    return true;
}

void TMediaPlaylistPrivate::ensureParser()
{
    if (parser)
        return;

    parser = new TPlaylistFileParser(q_ptr);
    QObject::connect(parser, &TPlaylistFileParser::finished, [this]() { loadFinished(); });
    QObject::connect(parser, &TPlaylistFileParser::error,
                     [this](TMediaPlaylist::Error err, const QString &errorMsg) {
                         loadFailed(err, errorMsg);
                     });
}

QT_END_NAMESPACE