#include "mudletapplication.h"
#include <QEvent>
#include <QFileOpenEvent>

MudletApplication::MudletApplication(int& argc, char** argv)
    : QApplication(argc, argv)
{
}

bool MudletApplication::event(QEvent* event)
{
    if (event->type() == QEvent::FileOpen) {
        emit urlReceived(static_cast<QFileOpenEvent*>(event)->url());
        return true;
    }
    return QApplication::event(event);
}
