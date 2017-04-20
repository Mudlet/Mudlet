/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include <QCoreApplication>
#include <QQmlApplicationEngine>

#ifdef QT_STATIC
#include <QtPlugin>
Q_IMPORT_PLUGIN(CommuniPlugin)
#endif // QT_STATIC

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // enable debug output
    qputenv("IRC_DEBUG", "1");

    QQmlApplicationEngine engine(QUrl("qrc:/main.qml"));
    return app.exec();
}
