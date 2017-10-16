#ifndef UPDATER_H
#define UPDATER_H

#include "dblsqd/feed.h"
#include "dblsqd/update_dialog.h"

#include "pre_guard.h"
#include <QObject>
#include "post_guard.h"

class TUpdateDialog : public dblsqd::UpdateDialog
{
public:
    explicit TUpdateDialog(dblsqd::Feed* feed, Type type = dblsqd::UpdateDialog::Type::OnUpdateAvailable,
                           QWidget* parent = nullptr, QSettings* settings = new QSettings())
    : UpdateDialog(feed, type, parent, settings)
    {
    }

private:
    void startUpdate() override;
};

class Updater : public QObject
{
    Q_OBJECT

    Q_DISABLE_COPY(Updater)

public:
    explicit Updater(QObject* parent = nullptr);
    void doUpdates();

private:
    dblsqd::Feed* feed;
    TUpdateDialog* updateDialog;

    void setupEvents() const;
    void untarOnLinux(const QString& fileName) const;

signals:

public slots:

    void updateBinaryOnLinux() const;
};

#endif // UPDATER_H
