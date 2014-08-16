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

#include <IrcCodecPlugin>
#include "uchardet.h"

class UCharDetPlugin : public IrcCodecPlugin
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "Communi.IrcCodecInterface")
#endif

public:
    explicit UCharDetPlugin(QObject* parent = 0);
    virtual ~UCharDetPlugin();

    virtual QByteArray key() const;
    virtual QByteArray codecForData(const QByteArray& data);

private:
    uchardet_t ud;
};

UCharDetPlugin::UCharDetPlugin(QObject* parent) : IrcCodecPlugin(parent)
{
    ud = uchardet_new();
}

UCharDetPlugin::~UCharDetPlugin()
{
    uchardet_delete(ud);
}

QByteArray UCharDetPlugin::key() const
{
    return QByteArray("uchardet");
}

QByteArray UCharDetPlugin::codecForData(const QByteArray& data)
{
    uchardet_reset(ud);
    uchardet_handle_data(ud, data.constData(), data.length());
    uchardet_data_end(ud);
    return uchardet_get_charset(ud);
}

#include "plugin.moc"

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(uchardetplugin, UCharDetPlugin);
#endif
