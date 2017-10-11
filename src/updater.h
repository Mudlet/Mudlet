#ifndef UPDATER_H
#define UPDATER_H

#include "../3rdparty/dblsqd/feed.h"
#include "../3rdparty/dblsqd/update_dialog.h"

#include "pre_guard.h"
#include <QObject>
#include "post_guard.h"

class Updater : public QObject
{
    Q_OBJECT

    Q_DISABLE_COPY(Updater)

public:
    explicit Updater(QObject* parent = nullptr, bool mNoAutomaticUpdates = false);
    void doUpdates();

private:
    bool mNoAutomaticUpdates;
    dblsqd::Feed* feed;
    dblsqd::UpdateDialog* updateDialog;

    void silentlyUpdate() const;
    void untarOnLinux(const QString &fileName) const;

signals:

public slots:

};

#endif // UPDATER_H
