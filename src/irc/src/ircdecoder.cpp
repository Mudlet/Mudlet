/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#include "ircdecoder_p.h"
#include "irccodecplugin.h"
#include <QCoreApplication>
#include <QPluginLoader>
#include <QStringList>
#include <QDebug>
#include <QMap>
#include <QDir>

typedef QMap<QByteArray, IrcCodecPlugin*> IrcCodecPluginMap;
Q_GLOBAL_STATIC(IrcCodecPluginMap, irc_codec_plugins)

static bool irc_is_utf8(const QByteArray& utf8);
static QByteArray irc_plugin_key = qgetenv("COMMUNI_CODEC_PLUGIN");

COMMUNI_EXPORT void irc_set_codec_plugin(const QByteArray& key)
{
    irc_plugin_key = key;
}

COMMUNI_EXPORT bool irc_is_supported_encoding(const QByteArray& encoding)
{
    static QSet<QByteArray> codecs = QTextCodec::availableCodecs().toSet();
    return codecs.contains(encoding);
}

IrcDecoder::IrcDecoder()
{
    d.fallback = QTextCodec::codecForName("UTF-8");
}

IrcDecoder::~IrcDecoder()
{
}

QByteArray IrcDecoder::encoding() const
{
    return d.fallback->name();
}

void IrcDecoder::setEncoding(const QByteArray& encoding)
{
    d.fallback = QTextCodec::codecForName(encoding);
}

QString IrcDecoder::decode(const QByteArray& data) const
{
    // TODO: not thread safe
    static QByteArray pluginKey;
    static bool initialized = false;
    if (!initialized)
    {
        pluginKey = const_cast<IrcDecoder*>(this)->initialize();
        initialized = true;
    }

    QTextCodec* codec = 0;
    if (irc_is_utf8(data))
    {
        codec = QTextCodec::codecForName("UTF-8");
    }
    else
    {
        QByteArray name = d.fallback->name();
        IrcCodecPlugin* plugin = irc_codec_plugins()->value(pluginKey);
        if (plugin)
            name = plugin->codecForData(data);
        codec = QTextCodec::codecForName(name);
    }

    if (!codec)
        codec = d.fallback;
    Q_ASSERT(codec);
    return codec->toUnicode(data);
}

QByteArray IrcDecoder::initialize()
{
    bool loaded = loadPlugins();
    QByteArray pluginKey = irc_plugin_key;
    if (!pluginKey.isEmpty() && !irc_codec_plugins()->contains(pluginKey))
    {
        qWarning() << "IrcDecoder:" << pluginKey << "plugin not loaded";
        if (loaded)
            qWarning() << "IrcDecoder: available plugins:" << irc_codec_plugins()->keys();
    }
    if (!loaded)
        qWarning() << "IrcDecoder: no plugins available";

    if (pluginKey.isEmpty() && !irc_codec_plugins()->isEmpty())
        pluginKey = irc_codec_plugins()->keys().first();
    return pluginKey;
}

#if defined(Q_OS_WIN)
#define COMMUNI_PATH_SEPARATOR ';'
#else
#define COMMUNI_PATH_SEPARATOR ':'
#endif

static QStringList pluginPaths()
{
    QStringList paths = QCoreApplication::libraryPaths();
    const QByteArray env = qgetenv("COMMUNI_PLUGIN_PATH");
    if (!env.isEmpty())
    {
        foreach (const QString& path, QFile::decodeName(env).split(COMMUNI_PATH_SEPARATOR, QString::SkipEmptyParts))
        {
            QString canonicalPath = QDir(path).canonicalPath();
            if (!canonicalPath.isEmpty() && !paths.contains(canonicalPath))
                paths += canonicalPath;
        }
    }
    return paths;
}

bool IrcDecoder::loadPlugins()
{
    foreach (const QString& path, pluginPaths())
    {
        QDir dir(path);
        if (!dir.cd("communi"))
            continue;

        foreach (const QFileInfo& file, dir.entryInfoList(QDir::Files))
        {
            QPluginLoader loader(file.absoluteFilePath());
            IrcCodecPlugin* plugin = qobject_cast<IrcCodecPlugin*>(loader.instance());
            if (plugin)
                irc_codec_plugins()->insert(plugin->key(), plugin);
        }
    }
    return !irc_codec_plugins()->isEmpty();
}

/*
  The Original Code is mozilla.org code.
  See http://lxr.mozilla.org/mozilla/source/modules/rdf/src/utils.c

  Copyright (C) 1998 Netscape Communications Corporation
*/
#define kLeft1BitMask  0x80
#define kLeft2BitsMask 0xC0
#define kLeft3BitsMask 0xE0
#define kLeft4BitsMask 0xF0
#define kLeft5BitsMask 0xF8
#define kLeft6BitsMask 0xFC
#define kLeft7BitsMask 0xFE

#define k2BytesLeadByte kLeft2BitsMask
#define k3BytesLeadByte kLeft3BitsMask
#define k4BytesLeadByte kLeft4BitsMask
#define k5BytesLeadByte kLeft5BitsMask
#define k6BytesLeadByte kLeft6BitsMask
#define kTrialByte      kLeft1BitMask

#define UTF8_1Byte(c) ( 0 == ((c) & kLeft1BitMask))
#define UTF8_2Bytes(c) ( k2BytesLeadByte == ((c) & kLeft3BitsMask))
#define UTF8_3Bytes(c) ( k3BytesLeadByte == ((c) & kLeft4BitsMask))
#define UTF8_4Bytes(c) ( k4BytesLeadByte == ((c) & kLeft5BitsMask))
#define UTF8_5Bytes(c) ( k5BytesLeadByte == ((c) & kLeft6BitsMask))
#define UTF8_6Bytes(c) ( k6BytesLeadByte == ((c) & kLeft7BitsMask))
#define UTF8_ValidTrialByte(c) ( kTrialByte == ((c) & kLeft2BitsMask))

bool irc_is_utf8(const QByteArray& utf8)
{
    int clen = 0;
    for (int i = 0; i < utf8.length(); i += clen)
    {
        if (UTF8_1Byte(utf8[i]))
        {
            clen = 1;
        }
        else if (UTF8_2Bytes(utf8[i]))
        {
            clen = 2;
            // No enough trail bytes
            if ((i + clen) > utf8.length())
                return false;
            // 0000 0000 - 0000 007F : should encode in less bytes
            if (0 ==  (utf8[i] & 0x1E))
                return false;
        }
        else if (UTF8_3Bytes(utf8[i]))
        {
            clen = 3;
            // No enough trail bytes
            if ((i + clen) > utf8.length())
                return false;
            // a single Surrogate should not show in 3 bytes UTF8, instead,
            // the pair should be intepreted as one single UCS4 char and
            // encoded UTF8 in 4 bytes
            if ((0xED == utf8[i]) && (0xA0 == (utf8[i+1] & 0xA0)))
                return false;
            // 0000 0000 - 0000 07FF : should encode in less bytes
            if ((0 == (utf8[i] & 0x0F)) && (0 == (utf8[i+1] & 0x20)))
                return false;
        }
        else if (UTF8_4Bytes(utf8[i]))
        {
            clen = 4;
            // No enough trail bytes
            if ((i + clen) > utf8.length())
                return false;
            // 0000 0000 - 0000 FFFF : should encode in less bytes
            if ((0 == (utf8[i] & 0x07 )) && (0 == (utf8[i+1] & 0x30)))
                return false;
        }
        else if (UTF8_5Bytes(utf8[i]))
        {
            clen = 5;
            // No enough trail bytes
            if ((i + clen) > utf8.length())
                return false;
            // 0000 0000 - 001F FFFF : should encode in less bytes
            if ((0 == (utf8[i] & 0x03 )) && (0 == (utf8[i+1] & 0x38)))
                return false;
        }
        else if (UTF8_6Bytes(utf8[i]))
        {
            clen = 6;
            // No enough trail bytes
            if ((i + clen) > utf8.length())
                return false;
            // 0000 0000 - 03FF FFFF : should encode in less bytes
            if ((0 == (utf8[i] & 0x01)) && (0 == (utf8[i+1] & 0x3E)))
                return false;
        }
        else
        {
            return false;
        }
        for (int j = 1; j < clen; ++j)
        {
            if (!UTF8_ValidTrialByte(utf8[i+j])) // Trail bytes invalid
                return false;
        }
    }
    return true;
}
