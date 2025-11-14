#ifndef MUDLET_MUDLETAPPLICATION_H
#define MUDLET_MUDLETAPPLICATION_H

#include <QApplication>

class MudletApplication : public QApplication
{
    Q_OBJECT

public:
    MudletApplication(int& argc, char** argv);

protected:
    bool event(QEvent* event) override;

signals:
    void urlReceived(const QUrl& url);
};

#endif // MUDLET_MUDLETAPPLICATION_H
