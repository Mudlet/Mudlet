#ifndef MACOSANNOUNCER_H
#define MACOSANNOUNCER_H

#include <QObject>

class MacOSAnnouncer : public QObject
{
    Q_OBJECT
public:
    explicit MacOSAnnouncer(QObject *parent = nullptr);
    static void announce(QString text);

signals:

};

#endif // MACOSANNOUNCER_H
