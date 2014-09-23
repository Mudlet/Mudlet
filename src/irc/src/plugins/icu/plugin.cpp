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
#include <unicode/ucsdet.h>

class IcuPlugin : public IrcCodecPlugin
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "Communi.IrcCodecInterface")
#endif

public:
    explicit IcuPlugin(QObject* parent = 0);
    virtual ~IcuPlugin();

    virtual QByteArray key() const;
    virtual QByteArray codecForData(const QByteArray& data);

private:
    UCharsetDetector* ucsd;
};

IcuPlugin::IcuPlugin(QObject* parent) : IrcCodecPlugin(parent)
{
    UErrorCode status = U_ZERO_ERROR;
    ucsd = ucsdet_open(&status);
    if (U_FAILURE(status))
        qWarning("IcuPlugin: ICU initialization failed: %s", u_errorName(status));
}

IcuPlugin::~IcuPlugin()
{
    ucsdet_close(ucsd);
}

QByteArray IcuPlugin::key() const
{
    return QByteArray("icu");
}

QByteArray IcuPlugin::codecForData(const QByteArray& data)
{
    QByteArray encoding;
    UErrorCode status = U_ZERO_ERROR;
    if (ucsd)
    {
        ucsdet_setText(ucsd, data.constData(), data.length(), &status);
        if (!U_FAILURE(status))
        {
            const UCharsetMatch* match = ucsdet_detect(ucsd, &status);
            if (match && !U_FAILURE(status))
                encoding = ucsdet_getName(match, &status);
        }
    }
    if (U_FAILURE(status))
        qWarning("IcuPlugin::codecForData() failed: %s", u_errorName(status));
    return encoding;
}

#include "plugin.moc"

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(icuplugin, IcuPlugin);
#endif
