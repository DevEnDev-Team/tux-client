#ifndef NOTESMANAGER_H
#define NOTESMANAGER_H

#include <QObject>
#include <vector>
#include <memory>
#include <map>
#include "notemodel.h"
#include "istorageprovider.h"
#include "stickywindow.h"

#include <QSystemTrayIcon>
#include <QTimer>

class NotesListWindow;
class SyncManager;

class NotesManager : public QObject {
    Q_OBJECT
public:
    explicit NotesManager(std::unique_ptr<IStorageProvider> storageProvider, QObject *parent = nullptr);
    ~NotesManager();

    void start();

private slots:
    void onNoteChanged(const QString& id);
    void onNewNoteRequested();
    void onMobileSyncRequested();
    void onDuplicateNoteRequested(const NoteModel& model);
    void onDeleteNoteRequested(const QString& id);
    void onShowNotesListRequested();
    void onOpenNoteFromListRequested(const QString& id);
    void onDeleteNoteFromListRequested(const QString& id);
    void onLogoutRequested();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void syncWithServer();

private:
    void createNoteWindow(const NoteModel& model);
    void createDefaultNote();
    void saveAll();
    void setupTrayIcon();
    void updateWindowsFromNotes();
    QString generateUniqueId() const;

    std::unique_ptr<IStorageProvider> m_storage;
    std::unique_ptr<SyncManager> m_sync;
    std::vector<NoteModel> m_notes;
    std::map<QString, StickyWindow*> m_windows;
    NotesListWindow* m_listWindow = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QTimer* m_syncTimer = nullptr;
    QStringList m_deletedNotesPendingSync;
};

#endif // NOTESMANAGER_H
