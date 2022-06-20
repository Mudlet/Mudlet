#ifndef ANNOUNCER_H
#define ANNOUNCER_H

#include <QObject>

class Announcer : public QObject
{
    Q_OBJECT
public:
    explicit Announcer(QObject *parent = nullptr);
    static void announce(QString text);

signals:

};

#endif // ANNOUNCER_H
