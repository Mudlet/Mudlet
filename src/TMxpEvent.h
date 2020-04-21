//
// Created by gustavo on 21/04/2020.
//

#ifndef MUDLET_SRC_TMXPEVENT_H
#define MUDLET_SRC_TMXPEVENT_H

#include <QStringList>
#include <QMap>

struct TMxpEvent {
    QString name;
    QMap<QString, QString> attrs;
    QStringList actions;

};

#endif //MUDLET_SRC_TMXPEVENT_H
