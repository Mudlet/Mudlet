#ifndef DEFAULTCLIENTUTILS_H
#define DEFAULTCLIENTUTILS_H
#include <QString>

QString getCurrentTelnetOpenCommand();
void setCurrentExecutableAsTelnetOpenCommand();
QString commandForCurrentExecutable();
bool isCurrentExecutableDefault();

#endif //DEFAULTCLIENTUTILS_H
