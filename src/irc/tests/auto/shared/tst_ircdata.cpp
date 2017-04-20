/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "tst_ircdata.h"
#include "tst_freenode.h"
#include "tst_ircnet.h"
#include "tst_euirc.h"

QList<QByteArray> tst_IrcData::keys()
{
    return QList<QByteArray>() << "freenode" << "ircnet" << "euirc";
}

QByteArray tst_IrcData::welcome(const QByteArray& key)
{
    static QHash<QByteArray, QByteArray> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", freenode_welcome);
        blobs.insert("ircnet", ircnet_welcome);
        blobs.insert("euirc", euirc_welcome);
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}

QByteArray tst_IrcData::join(const QByteArray& key)
{
    static QHash<QByteArray, QByteArray> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", freenode_join);
        blobs.insert("ircnet", ircnet_join);
        blobs.insert("euirc", euirc_join);
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}

QStringList tst_IrcData::names(const QByteArray& key)
{
    static QHash<QByteArray, QStringList> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", QString::fromUtf8(freenode_names).split(" "));
        blobs.insert("ircnet", QString::fromUtf8(ircnet_names).split(" "));
        blobs.insert("euirc", QString::fromUtf8(euirc_names).split(" "));
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}

QStringList tst_IrcData::admins(const QByteArray& key)
{
    static QHash<QByteArray, QStringList> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", QString::fromUtf8(freenode_admins).split(" "));
        blobs.insert("ircnet", QString::fromUtf8(ircnet_admins).split(" "));
        blobs.insert("euirc", QString::fromUtf8(euirc_admins).split(" "));
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}

QStringList tst_IrcData::ops(const QByteArray& key)
{
    static QHash<QByteArray, QStringList> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", QString::fromUtf8(freenode_ops).split(" "));
        blobs.insert("ircnet", QString::fromUtf8(ircnet_ops).split(" "));
        blobs.insert("euirc", QString::fromUtf8(euirc_ops).split(" "));
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}

QStringList tst_IrcData::halfops(const QByteArray& key)
{
    static QHash<QByteArray, QStringList> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", QString::fromUtf8(freenode_halfops).split(" "));
        blobs.insert("ircnet", QString::fromUtf8(ircnet_halfops).split(" "));
        blobs.insert("euirc", QString::fromUtf8(euirc_halfops).split(" "));
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}

QStringList tst_IrcData::voices(const QByteArray& key)
{
    static QHash<QByteArray, QStringList> blobs;
    if (blobs.isEmpty()) {
        blobs.insert("freenode", QString::fromUtf8(freenode_voices).split(" "));
        blobs.insert("ircnet", QString::fromUtf8(ircnet_voices).split(" "));
        blobs.insert("euirc", QString::fromUtf8(euirc_voices).split(" "));
    }
    return blobs.value(key.isEmpty() ? keys().first() : key);
}
