#ifndef UPDATER_H
#define UPDATER_H


#include "pre_guard.h"
#include <QObject>
#include "post_guard.h"

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);
    void doUpdates();

private:
    void silentlyUpdate();

signals:

public slots:
};

#endif // UPDATER_H
