#include <QApplication>
#include <memory>
#include <QIcon>
#include "localstorageprovider.h"
#include "notesmanager.h"
#include "stickywindow.h"

int main(int argc, char *argv[]) {
    // Option pour exporter l'icône de l'application (utilisé par le script d'installation)
    if (argc > 2 && QString(argv[1]) == "--export-icon") {
        QApplication app(argc, argv);
        QIcon icon = StickyWindow::createFoxIcon();
        QPixmap pixmap = icon.pixmap(256, 256);
        return pixmap.save(argv[2]) ? 0 : 1;
    }

    // Forcer XWayland pour que xprop puisse gérer le premier plan sous COSMIC
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication app(argc, argv);
    
    app.setApplicationName("Tux-It");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DevEnDev");
    app.setWindowIcon(StickyWindow::createFoxIcon());

    auto storage = std::make_unique<LocalStorageProvider>();
    NotesManager manager(std::move(storage));
    manager.start();

    return app.exec();
}
