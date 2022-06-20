#include "Announcer.h"

#include <QDebug>

Announcer::Announcer(QObject *parent)
: QObject{parent}
{

}

void Announcer::announce(QString text)
{
    qDebug() << "announcing" << text;
}
