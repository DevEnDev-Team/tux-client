#ifndef AUTOSTARTMANAGER_H
#define AUTOSTARTMANAGER_H

#include <QString>
#include <QIcon>

class AutostartManager {
public:
    static bool isEnabled();
    static bool setEnabled(bool enable);
    static bool toggle();
    static QIcon createAutostartIcon(bool enabled, bool darkTheme);
    static QString getAutostartFilePath();
};

#endif // AUTOSTARTMANAGER_H
