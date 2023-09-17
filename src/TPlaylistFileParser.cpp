// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediametadata.h"
#include "TMedia.h"
#include "TPlaylistFileParser.h"

#include <QDebug>
#include <QFileInfo>
#include <QIODevice>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>

QT_BEGIN_NAMESPACE

namespace {

class ParserBase
{
public:
    explicit ParserBase(TPlaylistFileParser *parent) : m_parent(parent), m_aborted(false)
    {
        Q_ASSERT(m_parent);
    }

    bool parseLine(int lineIndex, const QString &line, const QUrl &root)
    {
        if (m_aborted)
            return false;

        const bool ok = parseLineImpl(lineIndex, line, root);
        return ok && !m_aborted;
    }

    virtual void abort() { m_aborted = true; }
    virtual ~ParserBase() = default;

protected:
    virtual bool parseLineImpl(int lineIndex, const QString &line, const QUrl &root) = 0;

    static QUrl expandToFullPath(const QUrl &root, const QString &line)
    {
        // On Linux, backslashes are not converted to forward slashes :/
        if (line.startsWith(QLatin1String("//")) || line.startsWith(QLatin1String("\\\\"))) {
            // Network share paths are not resolved
            return QUrl::fromLocalFile(line);
        }

        QUrl url(line);
        if (url.scheme().isEmpty()) {
            // Resolve it relative to root
            if (root.isLocalFile())
                return QUrl::fromUserInput(line, root.adjusted(QUrl::RemoveFilename).toLocalFile(),
                                           QUrl::AssumeLocalFile);
            return root.resolved(url);
        }
        if (url.scheme().length() == 1)
            // Assume it's a drive letter for a Windows path
            url = QUrl::fromLocalFile(line);

        return url;
    }

    void newItemFound(const QVariant &content) { Q_EMIT m_parent->newItem(content); }

    TPlaylistFileParser *m_parent;
    bool m_aborted;
};

class M3UParser : public ParserBase
{
public:
    explicit M3UParser(TPlaylistFileParser *q) : ParserBase(q), m_extendedFormat(false) { }

    /*
     *
    Extended M3U directives

    #EXTM3U - header - must be first line of file
    #EXTINF - extra info - length (seconds), title
    #EXTINF - extra info - length (seconds), artist '-' title

    Example

    #EXTM3U
    #EXTINF:123, Sample artist - Sample title
    C:\Documents and Settings\I\My Music\Sample.mp3
    #EXTINF:321,Example Artist - Example title
    C:\Documents and Settings\I\My Music\Greatest Hits\Example.ogg

     */
    bool parseLineImpl(int lineIndex, const QString &line, const QUrl &root) override
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (line[0] == u'#') {
            if (m_extendedFormat) {
                if (line.startsWith(QLatin1String("#EXTINF:"))) {
                    m_extraInfo.clear();

                    int artistStart = line.indexOf(QLatin1String(","), 8);
                    bool ok = false;
                    QStringView lineView{ line };
                    int length = lineView.mid(8, artistStart < 8 ? -1 : artistStart - 8)
                                         .trimmed()
                                         .toInt(&ok);
                    if (ok && length > 0) {
                        // convert from second to milisecond
                        m_extraInfo[QMediaMetaData::Duration] = QVariant(length * 1000);
                    }
                    if (artistStart > 0) {
                        int titleStart = getSplitIndex(line, artistStart);
                        if (titleStart > artistStart) {
                            m_extraInfo[QMediaMetaData::Author] =
                                    lineView.mid(artistStart + 1, titleStart - artistStart - 1)
                                            .trimmed()
                                            .toString()
                                            .replace(QLatin1String("--"), QLatin1String("-"));
                            m_extraInfo[QMediaMetaData::Title] =
                                    lineView.mid(titleStart + 1)
                                            .trimmed()
                                            .toString()
                                            .replace(QLatin1String("--"), QLatin1String("-"));
                        } else {
                            m_extraInfo[QMediaMetaData::Title] =
                                    lineView.mid(artistStart + 1)
                                            .trimmed()
                                            .toString()
                                            .replace(QLatin1String("--"), QLatin1String("-"));
                        }
                    }
                }
            } else if (lineIndex == 0 && line.startsWith(QLatin1String("#EXTM3U"))) {
                m_extendedFormat = true;
            }
        } else {
            QUrl url = expandToFullPath(root, line);
            m_extraInfo[QMediaMetaData::Url] = url;
            m_parent->playlist.append(url);
            newItemFound(QVariant::fromValue(m_extraInfo));
            m_extraInfo.clear();
        }
#endif

        return true;
    }

    int getSplitIndex(const QString &line, int startPos)
    {
        if (startPos < 0)
            startPos = 0;
        const QChar *buf = line.data();
        for (int i = startPos; i < line.length(); ++i) {
            if (buf[i] == u'-') {
                if (i == line.length() - 1)
                    return i;
                ++i;
                if (buf[i] != u'-')
                    return i - 1;
            }
        }
        return -1;
    }

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMediaMetaData m_extraInfo;
#endif
    bool m_extendedFormat;
};

class PLSParser : public ParserBase
{
public:
    explicit PLSParser(TPlaylistFileParser *q) : ParserBase(q) { }

    /*
     *
    The format is essentially that of an INI file structured as follows:

    Header

        * [playlist] : This tag indicates that it is a Playlist File

    Track Entry
    Assuming track entry #X

        * FileX : Variable defining location of stream.
        * TitleX : Defines track title.
        * LengthX : Length in seconds of track. Value of -1 indicates indefinite.

    Footer

        * NumberOfEntries : This variable indicates the number of tracks.
        * Version : Playlist version. Currently only a value of 2 is valid.

    [playlist]

    File1=Alternative\everclear - SMFTA.mp3

    Title1=Everclear - So Much For The Afterglow

    Length1=233

    File2=http://www.site.com:8000/listen.pls

    Title2=My Cool Stream

    Length5=-1

    NumberOfEntries=2

    Version=2
    */
    bool parseLineImpl(int, const QString &line, const QUrl &root) override
    {
        // We ignore everything but 'File' entries, since that's the only thing we care about.
        if (!line.startsWith(QLatin1String("File")))
            return true;

        QString value = getValue(line);
        if (value.isEmpty())
            return true;

        QUrl path = expandToFullPath(root, value);
        m_parent->playlist.append(path);
        newItemFound(path);

        return true;
    }

    QString getValue(QStringView line)
    {
        int start = line.indexOf(u'=');
        if (start < 0)
            return QString();
        return line.mid(start + 1).trimmed().toString();
    }
};
}

/////////////////////////////////////////////////////////////////////////////////////////////////

class TPlaylistFileParserPrivate
{
    Q_DECLARE_PUBLIC(TPlaylistFileParser)
public:
    TPlaylistFileParserPrivate(TPlaylistFileParser *q)
        : q_ptr(q),
          m_stream(nullptr),
          m_type(TPlaylistFileParser::UNKNOWN),
          m_scanIndex(0),
          m_lineIndex(-1),
          m_utf8(false),
          m_aborted(false)
    {
    }

    void handleData();
    void handleParserFinished();
    void abort();
    void reset();

    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> m_source;
    QScopedPointer<ParserBase> m_currentParser;
    QByteArray m_buffer;
    QUrl m_root;
    QNetworkAccessManager m_mgr;
    QString m_mimeType;
    TPlaylistFileParser *q_ptr;
    QPointer<QIODevice> m_stream;
    TPlaylistFileParser::FileType m_type;
    struct ParserJob
    {
        QIODevice *m_stream;
        QUrl m_media;
        QString m_mimeType;
        [[nodiscard]] bool isValid() const { return m_stream || !m_media.isEmpty(); }
        void reset()
        {
            m_stream = nullptr;
            m_media = QUrl();
            m_mimeType = QString();
        }
    } m_pendingJob;
    int m_scanIndex;
    int m_lineIndex;
    bool m_utf8;
    bool m_aborted;

private:
    bool processLine(int startIndex, int length);
};

#define LINE_LIMIT 4096
#define READ_LIMIT 64

bool TPlaylistFileParserPrivate::processLine(int startIndex, int length)
{
    Q_Q(TPlaylistFileParser);
    m_lineIndex++;

    if (!m_currentParser) {
        const QString urlString = m_root.toString();
        const QString &suffix = !urlString.isEmpty() ? QFileInfo(urlString).suffix() : urlString;
        QString mimeType;
        if (m_source)
            mimeType = m_source->header(QNetworkRequest::ContentTypeHeader).toString();
        m_type = TPlaylistFileParser::findPlaylistType(
                suffix, !mimeType.isEmpty() ? mimeType : m_mimeType, m_buffer.constData(),
                quint32(m_buffer.size()));

        switch (m_type) {
        case TPlaylistFileParser::UNKNOWN:
            emit q->error(TMediaPlaylist::FormatError,
                          TMediaPlaylist::tr("%1 playlist type is unknown").arg(m_root.toString()));
            q->abort();
            return false;
        case TPlaylistFileParser::M3U:
            m_currentParser.reset(new M3UParser(q));
            break;
        case TPlaylistFileParser::M3U8:
            m_currentParser.reset(new M3UParser(q));
            m_utf8 = true;
            break;
        case TPlaylistFileParser::PLS:
            m_currentParser.reset(new PLSParser(q));
            break;
        }

        Q_ASSERT(!m_currentParser.isNull());
    }

    QString line;

    if (m_utf8) {
        line = QString::fromUtf8(m_buffer.constData() + startIndex, length).trimmed();
    } else {
        line = QString::fromLatin1(m_buffer.constData() + startIndex, length).trimmed();
    }
    if (line.isEmpty())
        return true;

    Q_ASSERT(m_currentParser);
    return m_currentParser->parseLine(m_lineIndex, line, m_root);
}

void TPlaylistFileParserPrivate::handleData()
{
    Q_Q(TPlaylistFileParser);
    while (m_stream->bytesAvailable() && !m_aborted) {
        int expectedBytes =
                qMin(READ_LIMIT,
                     int(qMin(m_stream->bytesAvailable(), qint64(LINE_LIMIT - m_buffer.size()))));
        m_buffer.push_back(m_stream->read(expectedBytes));
        int processedBytes = 0;
        while (m_scanIndex < m_buffer.length() && !m_aborted) {
            char s = m_buffer[m_scanIndex];
            if (s == '\r' || s == '\n') {
                int l = m_scanIndex - processedBytes;
                if (l > 0) {
                    if (!processLine(processedBytes, l))
                        break;
                }
                processedBytes = m_scanIndex + 1;
                if (!m_stream) {
                    // some error happened, so exit parsing
                    return;
                }
            }
            m_scanIndex++;
        }

        if (m_aborted)
            break;

        if (m_buffer.length() - processedBytes >= LINE_LIMIT) {
            emit q->error(TMediaPlaylist::FormatError,
                          TMediaPlaylist::tr("invalid line in playlist file"));
            q->abort();
            break;
        }

        if (!m_stream->bytesAvailable() && (!m_source || !m_source->isFinished())) {
            // last line
            processLine(processedBytes, -1);
            break;
        }

        Q_ASSERT(m_buffer.length() == m_scanIndex);
        if (processedBytes == 0)
            continue;

        int copyLength = m_buffer.length() - processedBytes;
        if (copyLength > 0) {
            Q_ASSERT(copyLength <= READ_LIMIT);
            m_buffer = m_buffer.right(copyLength);
        } else {
            m_buffer.clear();
        }
        m_scanIndex = 0;
    }

    handleParserFinished();
}

TPlaylistFileParser::TPlaylistFileParser(QObject *parent)
    : QObject(parent), d_ptr(new TPlaylistFileParserPrivate(this))
{
}

TPlaylistFileParser::~TPlaylistFileParser() = default;

TPlaylistFileParser::FileType TPlaylistFileParser::findByMimeType(const QString &mime)
{
    if (mime == QLatin1String("text/uri-list") || mime == QLatin1String("audio/x-mpegurl")
        || mime == QLatin1String("audio/mpegurl"))
        return TPlaylistFileParser::M3U;

    if (mime == QLatin1String("application/x-mpegURL")
        || mime == QLatin1String("application/vnd.apple.mpegurl"))
        return TPlaylistFileParser::M3U8;

    if (mime == QLatin1String("audio/x-scpls"))
        return TPlaylistFileParser::PLS;

    return TPlaylistFileParser::UNKNOWN;
}

TPlaylistFileParser::FileType TPlaylistFileParser::findBySuffixType(const QString &suffix)
{
    const QString &s = suffix.toLower();

    if (s == QLatin1String("m3u"))
        return TPlaylistFileParser::M3U;

    if (s == QLatin1String("m3u8"))
        return TPlaylistFileParser::M3U8;

    if (s == QLatin1String("pls"))
        return TPlaylistFileParser::PLS;

    return TPlaylistFileParser::UNKNOWN;
}

TPlaylistFileParser::FileType TPlaylistFileParser::findByDataHeader(const char *data, quint32 size)
{
    if (!data || size == 0)
        return TPlaylistFileParser::UNKNOWN;

    if (size >= 7 && strncmp(data, "#EXTM3U", 7) == 0)
        return TPlaylistFileParser::M3U;

    if (size >= 10 && strncmp(data, "[playlist]", 10) == 0)
        return TPlaylistFileParser::PLS;

    return TPlaylistFileParser::UNKNOWN;
}

TPlaylistFileParser::FileType TPlaylistFileParser::findPlaylistType(const QString &suffix,
                                                                    const QString &mime,
                                                                    const char *data, quint32 size)
{

    FileType dataHeaderType = findByDataHeader(data, size);
    if (dataHeaderType != UNKNOWN)
        return dataHeaderType;

    FileType mimeType = findByMimeType(mime);
    if (mimeType != UNKNOWN)
        return mimeType;

    mimeType = findBySuffixType(mime);
    if (mimeType != UNKNOWN)
        return mimeType;

    FileType suffixType = findBySuffixType(suffix);
    if (suffixType != UNKNOWN)
        return suffixType;

    return UNKNOWN;
}

/*
 * Delegating
 */
void TPlaylistFileParser::start(const QUrl &media, QIODevice *stream, const QString &mimeType)
{
    if (stream)
        start(stream, mimeType);
    else
        start(media, mimeType);
}

void TPlaylistFileParser::start(QIODevice *stream, const QString &mimeType)
{
    Q_D(TPlaylistFileParser);
    const bool validStream = stream ? (stream->isOpen() && stream->isReadable()) : false;

    if (!validStream) {
        Q_EMIT error(TMediaPlaylist::AccessDeniedError, TMediaPlaylist::tr("Invalid stream"));
        return;
    }

    if (!d->m_currentParser.isNull()) {
        abort();
        d->m_pendingJob = { stream, QUrl(), mimeType };
        return;
    }

    playlist.clear();
    d->reset();
    d->m_mimeType = mimeType;
    d->m_stream = stream;
    connect(d->m_stream, SIGNAL(readyRead()), this, SLOT(handleData()));
    d->handleData();
}

void TPlaylistFileParser::start(const QUrl &request, const QString &mimeType)
{
    Q_D(TPlaylistFileParser);
    const QUrl &url = request.url();

    if (url.isLocalFile() && !QFile::exists(url.toLocalFile())) {
        emit error(TMediaPlaylist::AccessDeniedError,
                   QString(TMediaPlaylist::tr("%1 does not exist")).arg(url.toString()));
        return;
    }

    if (!d->m_currentParser.isNull()) {
        abort();
        d->m_pendingJob = { nullptr, request, mimeType };
        return;
    }

    d->reset();
    d->m_root = url;
    d->m_mimeType = mimeType;
    d->m_source.reset(d->m_mgr.get(QNetworkRequest(request)));
    d->m_stream = d->m_source.get();
    connect(d->m_source.data(), SIGNAL(readyRead()), this, SLOT(handleData()));
    connect(d->m_source.data(), SIGNAL(finished()), this, SLOT(handleData()));
    connect(d->m_source.data(), SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this,
            SLOT(handleError()));

    if (url.isLocalFile())
        d->handleData();
}

void TPlaylistFileParser::abort()
{
    Q_D(TPlaylistFileParser);
    d->abort();

    if (d->m_source)
        d->m_source->disconnect();

    if (d->m_stream)
        disconnect(d->m_stream, SIGNAL(readyRead()), this, SLOT(handleData()));

    playlist.clear();
}

void TPlaylistFileParser::handleData()
{
    Q_D(TPlaylistFileParser);
    d->handleData();
}

void TPlaylistFileParserPrivate::handleParserFinished()
{
    Q_Q(TPlaylistFileParser);
    const bool isParserValid = !m_currentParser.isNull();
    if (!isParserValid && !m_aborted)
        emit q->error(TMediaPlaylist::FormatNotSupportedError,
                      TMediaPlaylist::tr("Empty file provided"));

    if (isParserValid && !m_aborted) {
        m_currentParser.reset();
        emit q->finished();
    }

    if (!m_aborted)
        q->abort();

    if (!m_source.isNull())
        m_source.reset();

    if (m_pendingJob.isValid())
        q->start(m_pendingJob.m_media, m_pendingJob.m_stream, m_pendingJob.m_mimeType);
}

void TPlaylistFileParserPrivate::abort()
{
    m_aborted = true;
    if (!m_currentParser.isNull())
        m_currentParser->abort();
}

void TPlaylistFileParserPrivate::reset()
{
    Q_ASSERT(m_currentParser.isNull());
    Q_ASSERT(m_source.isNull());
    m_buffer.clear();
    m_root.clear();
    m_mimeType.clear();
    m_stream = nullptr;
    m_type = TPlaylistFileParser::UNKNOWN;
    m_scanIndex = 0;
    m_lineIndex = -1;
    m_utf8 = false;
    m_aborted = false;
    m_pendingJob.reset();
}

void TPlaylistFileParser::handleError()
{
    Q_D(TPlaylistFileParser);
    const QString &errorString = d->m_source->errorString();
    Q_EMIT error(TMediaPlaylist::NetworkError, errorString);
    abort();
}

QT_END_NAMESPACE
