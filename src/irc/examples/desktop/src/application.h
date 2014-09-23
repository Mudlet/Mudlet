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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include "settings.h"

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char* argv[]);
    ~Application();

    static QString applicationSlogan();

    static QByteArray encoding();
    static void setEncoding(const QByteArray& encoding);

    static Settings settings();
    static void setSettings(const Settings& settings);

public slots:
    static void aboutApplication();
    static void aboutOxygen();
    static void showSettings();

signals:
    void settingsChanged(const Settings& settings);

private:
    struct ApplicationData
    {
        static QByteArray encoding;
        static Settings settings;
    };
};

#endif // APPLICATION_H
