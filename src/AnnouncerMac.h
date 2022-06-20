#ifndef MACOSANNOUNCER_H
#define MACOSANNOUNCER_H

#include <QObject>

class AnnouncerMac : public QObject
{
    Q_OBJECT
public:
    explicit AnnouncerMac(QObject *parent = nullptr);
    static void announce(QString text);

signals:

};

#endif // MACOSANNOUNCER_H
