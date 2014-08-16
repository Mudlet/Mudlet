/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "settings.h"
#include <QSettings>
#include <QTime>

Settings* Settings::instance()
{
    static Settings settings;
    return &settings;
}

Settings::Settings(QObject *parent) : QObject(parent)
{
    qsrand(QTime::currentTime().msec());
    qRegisterMetaTypeStreamOperators<ConnectionInfo>("ConnectionInfo");
    qRegisterMetaTypeStreamOperators<ConnectionInfos>("ConnectionInfos");
}

QString Settings::host() const
{ return QSettings().value("host", QString("irc.freenode.net")).toString(); }

void Settings::setHost(const QString& host)
{ QSettings().setValue("host", host); }

int Settings::port() const
{ return QSettings().value("port", 6667).toInt(); }

void Settings::setPort(int port)
{ QSettings().setValue("port", port); }

bool Settings::isSecure() const
{ return QSettings().value("secure", false).toBool(); }

void Settings::setSecure(bool secure)
{ QSettings().setValue("secure", secure); }

QString Settings::name() const
{ return QSettings().value("name", QString("Guest%1").arg(qrand() % 9999)).toString(); }

void Settings::setName(const QString& name)
{ QSettings().setValue("name", name); }

QString Settings::user() const
{ return QSettings().value("user").toString(); }

void Settings::setUser(const QString& user)
{ QSettings().setValue("user", user); }

QString Settings::real() const
{ return QSettings().value("real").toString(); }

void Settings::setReal(const QString& real)
{ QSettings().setValue("real", real); }

QString Settings::channel() const
{ return QSettings().value("channel", QString("#freenode")).toString(); }

void Settings::setChannel(const QString& channel)
{ QSettings().setValue("channel", channel); }

ConnectionInfos Settings::connections() const
{ return QSettings().value("connections").value<ConnectionInfos>(); }

void Settings::setConnections(const ConnectionInfos& connections)
{ QSettings().setValue("connections", QVariant::fromValue(connections)); }

bool Settings::timeStamp() const
{ return QSettings().value("timestamp", true).toBool(); }

void Settings::setTimeStamp(bool timeStamp)
{ QSettings().setValue("timestamp", timeStamp); }

bool Settings::stripNicks() const
{ return QSettings().value("stripNicks", true).toBool(); }

void Settings::setStripNicks(bool strip)
{ QSettings().setValue("stripNicks", strip); }

bool Settings::policyAgreed() const
{ return QSettings().value("policy", false).toBool(); }

void Settings::setPolicyAgreed(bool agreed)
{ QSettings().setValue("policy", agreed); }
