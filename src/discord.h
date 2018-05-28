#ifndef DISCORD_H
#define DISCORD_H

#include "pre_guard.h"
#include <QObject>
#include <QDebug>
#include "post_guard.h"

class Discord : public QObject
{
    Q_OBJECT
public:
    explicit Discord(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DISCORD_H
