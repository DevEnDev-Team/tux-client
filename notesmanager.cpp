#include "notesmanager.h"
#include "noteslistwindow.h"
#include "syncmanager.h"
#include "setupwizard.h"
#include <QUuid>
#include <QDateTime>
#include <QScreen>
#include <QApplication>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

NotesManager::NotesManager(std::unique_ptr<IStorageProvider> storageProvider, QObject *parent)
    : QObject(parent)
    , m_storage(std::move(storageProvider))
    , m_sync(std::make_unique<SyncManager>(this))
{
    // Connecter le signal de fin de synchronisation asynchrone
    connect(m_sync.get(), &SyncManager::syncFinished, this, [this](bool success) {
        if (success) {
            // Marquer toutes les notes locales comme synchronisées
            for (auto& note : m_notes) {
                note.synced = true;
            }
            m_storage->saveNotes(m_notes);

            // Vider les suppressions en attente puisque la propagation a réussi
            m_deletedNotesPendingSync.clear();

            // Mettre à jour l'affichage sur toutes les fenêtres ouvertes
            for (auto& pair : m_windows) {
                pair.second->setSynced(true);
            }

            // Mettre à jour la liste si elle est affichée
            if (m_listWindow) {
                m_listWindow->updateNotesList(m_notes);
            }
        }
    });
}

NotesManager::~NotesManager() {
    for (auto& pair : m_windows) {
        delete pair.second;
    }
}

void NotesManager::start() {
    // Si l'application n'est pas encore configurée, lancer l'assistant d'installation
    if (!m_sync->isConfigured()) {
        SetupWizard wizard;
        if (wizard.exec() == QDialog::Accepted) {
            m_sync->saveConfig(wizard.serverUrl(), wizard.apiKey(), true);
        } else {
            QApplication::quit();
            return;
        }
    }

    m_notes = m_storage->loadNotes();
    bool hadNotes = !m_notes.empty();

    // Afficher immédiatement les notes locales existantes pour la réactivité
    for (const auto& note : m_notes) {
        if (!note.archived && !note.trashed) {
            createNoteWindow(note);
        }
    }

    if (hadNotes) {
        onShowNotesListRequested();
    }

    // Lancer la synchronisation réseau si configurée
    if (m_sync->isEnabled()) {
        // Faire un premier pull pour fusionner avec le serveur avant de décider s'il faut créer une note par défaut
        m_sync->pullNotes([this, hadNotes](const std::vector<NoteModel>& remoteNotes, bool success) {
            if (success) {
                // Fusionner les notes locales et distantes
                std::vector<NoteModel> merged = m_sync->mergeNotes(m_notes, remoteNotes);

                // Si tout est vide (local et distant) au tout premier démarrage, on crée la note de bienvenue
                if (merged.empty() && !hadNotes) {
                    NoteModel defaultNote;
                    defaultNote.id = generateUniqueId();
                    defaultNote.content = "Bienvenue dans vos Post-its !\n\nDouble-cliquez pour éditer.\nFaites un clic droit pour changer la couleur ou l'opacité.";
                    defaultNote.color = "#FFF59D"; // Jaune par défaut
                    defaultNote.position = QPoint(150, 150);
                    defaultNote.size = QSize(250, 250);
                    defaultNote.alwaysOnTop = false;
                    defaultNote.opacity = 1.0;
                    defaultNote.locked = false;
                    defaultNote.lastModified = QDateTime::currentDateTime();
                    defaultNote.created = defaultNote.lastModified;
                    defaultNote.synced = false;

                    merged.push_back(defaultNote);
                }

                // Détecter s'il y a des changements provenant du serveur ou locaux
                bool hasRemoteChanges = false;
                bool hasLocalChanges = false;

                if (merged.size() != m_notes.size()) {
                    hasRemoteChanges = true;
                } else {
                    for (size_t i = 0; i < m_notes.size(); ++i) {
                        if (m_notes[i] != merged[i]) {
                            hasRemoteChanges = true;
                            break;
                        }
                    }
                }

                if (merged.size() != remoteNotes.size()) {
                    hasLocalChanges = true;
                } else {
                    for (const auto& mNote : merged) {
                        auto rIt = std::find_if(remoteNotes.begin(), remoteNotes.end(), [&mNote](const NoteModel& rNote) {
                            return rNote.id == mNote.id;
                        });
                        if (rIt == remoteNotes.end() || mNote != *rIt || !mNote.synced) {
                            hasLocalChanges = true;
                            break;
                        }
                    }
                }

                if (hasRemoteChanges) {
                    m_notes = merged;
                    m_storage->saveNotes(m_notes);
                    updateWindowsFromNotes();
                    if (m_listWindow) {
                        m_listWindow->updateNotesList(m_notes);
                    }
                }

                if (hasLocalChanges) {
                    m_sync->pushNotes(m_notes);
                }
            } else {
                // Si la synchro initiale échoue et qu'on n'a aucune note locale du tout,
                // on crée la note par défaut
                if (!hadNotes && m_notes.empty()) {
                    createDefaultNote();
                }
            }

            // S'assurer qu'au moins une fenêtre ou la liste est visible
            if (m_windows.empty()) {
                if (m_notes.empty()) {
                    onNewNoteRequested();
                } else {
                    onShowNotesListRequested();
                }
            }
        });

        // Configurer la synchronisation automatique périodique toutes les 15 secondes
        m_syncTimer = new QTimer(this);
        connect(m_syncTimer, &QTimer::timeout, this, &NotesManager::syncWithServer);
        m_syncTimer->start(15000);
    } else {
        // En mode hors ligne, si aucune note n'existe du tout, créer la note par défaut
        if (!hadNotes && m_notes.empty()) {
            createDefaultNote();
        }

        if (m_windows.empty()) {
            onNewNoteRequested();
        }
    }

    setupTrayIcon();
}

void NotesManager::createNoteWindow(const NoteModel& note) {
    StickyWindow* window = new StickyWindow(note);
    m_windows[note.id] = window;

    connect(window, &StickyWindow::noteChanged, this, &NotesManager::onNoteChanged);
    connect(window, &StickyWindow::newNoteRequested, this, &NotesManager::onNewNoteRequested);
    connect(window, &StickyWindow::duplicateNoteRequested, this, &NotesManager::onDuplicateNoteRequested);
    connect(window, &StickyWindow::deleteNoteRequested, this, &NotesManager::onDeleteNoteRequested);
    connect(window, &StickyWindow::showNotesListRequested, this, &NotesManager::onShowNotesListRequested);
    connect(window, &StickyWindow::logoutRequested, this, &NotesManager::onLogoutRequested);

    window->show();
}

void NotesManager::createDefaultNote() {
    NoteModel defaultNote;
    defaultNote.id = generateUniqueId();
    defaultNote.content = "Bienvenue dans vos Post-its !\n\nDouble-cliquez pour éditer.\nFaites un clic droit pour changer la couleur ou l'opacité.";
    defaultNote.color = "#FFF59D"; // Jaune par défaut
    defaultNote.position = QPoint(150, 150);
    defaultNote.size = QSize(250, 250);
    defaultNote.alwaysOnTop = false;
    defaultNote.opacity = 1.0;
    defaultNote.locked = false;
    defaultNote.lastModified = QDateTime::currentDateTime();
    defaultNote.created = defaultNote.lastModified;
    defaultNote.synced = !m_sync->isEnabled(); // Non synchronisée si sync active pour être poussée au premier cycle
    
    m_notes.push_back(defaultNote);
    m_storage->saveNotes(m_notes);
    createNoteWindow(defaultNote);
}

void NotesManager::onNoteChanged(const QString& id) {
    StickyWindow* window = m_windows[id];
    if (!window) return;

    NoteModel updatedModel = window->getModel();
    updatedModel.synced = false; // Non synchronisé
    window->setSynced(false);
    
    // Update model in memory
    for (auto& note : m_notes) {
        if (note.id == id) {
            note = updatedModel;
            break;
        }
    }

    saveAll();
}

void NotesManager::onNewNoteRequested() {
    NoteModel newNote;
    newNote.id = generateUniqueId();
    newNote.content = "";
    newNote.color = "#FFF59D"; // Yellow
    
    // Position cascade relative to active window or screen center
    QPoint spawnPos(100, 100);
    if (!m_windows.empty()) {
        auto activeWin = m_windows.begin()->second;
        spawnPos = activeWin->pos() + QPoint(40, 40);
    }
    
    newNote.position = spawnPos;
    newNote.size = QSize(250, 250);
    newNote.alwaysOnTop = false;
    newNote.opacity = 1.0;
    newNote.locked = false;
    newNote.synced = false; // Nouvelle note
    newNote.lastModified = QDateTime::currentDateTime();
    newNote.created = newNote.lastModified;

    m_notes.push_back(newNote);
    createNoteWindow(newNote);
    saveAll();
}

void NotesManager::onDuplicateNoteRequested(const NoteModel& model) {
    NoteModel dupNote = model;
    dupNote.id = generateUniqueId();
    dupNote.position += QPoint(30, 30);
    dupNote.synced = false; // Nouvelle note dupliquée
    dupNote.lastModified = QDateTime::currentDateTime();
    dupNote.created = dupNote.lastModified;

    m_notes.push_back(dupNote);
    createNoteWindow(dupNote);
    saveAll();
}

void NotesManager::onDeleteNoteRequested(const QString& id) {
    auto winIt = m_windows.find(id);
    if (winIt != m_windows.end()) {
        winIt->second->close();
        delete winIt->second;
        m_windows.erase(winIt);
    }

    // Archiver la note au lieu de la supprimer définitivement
    for (auto it = m_notes.begin(); it != m_notes.end(); ++it) {
        if (it->id == id) {
            it->archived = true;
            break;
        }
    }

    saveAll();

    // Quitter l'application uniquement si aucune note active ne reste et que la liste n'est pas visible
    if (m_windows.empty() && (!m_listWindow || !m_listWindow->isVisible())) {
        QApplication::quit();
    }
}

void NotesManager::saveAll() {
    m_storage->saveNotes(m_notes);
    if (m_listWindow) {
        m_listWindow->updateNotesList(m_notes);
    }
    if (m_sync && m_sync->isEnabled()) {
        syncWithServer();
    }
}

QString NotesManager::generateUniqueId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void NotesManager::onShowNotesListRequested() {
    if (!m_listWindow) {
        m_listWindow = new NotesListWindow(m_notes);
        m_listWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_listWindow, &NotesListWindow::openNoteRequested, this, &NotesManager::onOpenNoteFromListRequested);
        connect(m_listWindow, &NotesListWindow::deleteNoteRequested, this, &NotesManager::onDeleteNoteFromListRequested);
        connect(m_listWindow, &QObject::destroyed, this, [this]() {
            m_listWindow = nullptr;
            if (m_windows.empty()) {
                QApplication::quit();
            }
        });
    }
    m_listWindow->show();
    m_listWindow->activateWindow();
    m_listWindow->raise();
}

void NotesManager::onOpenNoteFromListRequested(const QString& id) {
    // Si la fenêtre est déjà ouverte, l'activer
    auto winIt = m_windows.find(id);
    if (winIt != m_windows.end()) {
        winIt->second->show();
        winIt->second->activateWindow();
        winIt->second->raise();
        return;
    }

    // Sinon, trouver le modèle, le désarchiver et recréer la fenêtre
    for (auto& note : m_notes) {
        if (note.id == id) {
            note.archived = false; // Rendre à nouveau visible
            createNoteWindow(note);
            saveAll();
            break;
        }
    }
}

void NotesManager::onDeleteNoteFromListRequested(const QString& id) {
    // 1. Fermer et détruire la fenêtre si elle est ouverte à l'écran
    auto winIt = m_windows.find(id);
    if (winIt != m_windows.end()) {
        winIt->second->close();
        delete winIt->second;
        m_windows.erase(winIt);
    }

    // Enregistrer la suppression pour la propager lors de la prochaine synchro
    if (!m_deletedNotesPendingSync.contains(id)) {
        m_deletedNotesPendingSync.append(id);
    }

    // 2. Supprimer la note définitivement de la liste en mémoire
    for (auto it = m_notes.begin(); it != m_notes.end(); ++it) {
        if (it->id == id) {
            m_notes.erase(it);
            break;
        }
    }

    // 3. Sauvegarder sur le disque
    saveAll();

    // 4. Fermer l'application si aucune note active ne reste et que la liste n'est pas visible
    if (m_windows.empty() && (!m_listWindow || !m_listWindow->isVisible())) {
        QApplication::quit();
    }
}

void NotesManager::onLogoutRequested() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, "Déconnexion",
        "Voulez-vous vraiment vous déconnecter du serveur de synchronisation et passer en mode local ?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    // Vider les paramètres de synchro et désactiver
    m_sync->saveConfig("", "", false);

    // Marquer toutes les notes comme synchronisées (vu qu'on est en local désormais)
    for (auto& note : m_notes) {
        note.synced = true;
    }
    m_storage->saveNotes(m_notes);

    // Mettre à jour l'IHM des notes
    for (auto& pair : m_windows) {
        pair.second->setSynced(true);
        pair.second->updateAppearance(); // Force la dissimulation du nuage
    }

    // Rafraîchir la liste de notes si elle est visible
    if (m_listWindow) {
        m_listWindow->updateNotesList(m_notes);
    }

    QMessageBox::information(nullptr, "Mode local activé", "Vous êtes déconnecté. L'application tourne désormais en mode 100% local.");
}

void NotesManager::setupTrayIcon() {
    if (m_trayIcon) return;

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(StickyWindow::createFoxIcon());
    m_trayIcon->setToolTip("Post-It (Renard Mignon)");

    QMenu* trayMenu = new QMenu();
    QAction* listAction = trayMenu->addAction("Tableau de bord");
    QAction* newAction = trayMenu->addAction("Nouveau Post-It");
    trayMenu->addSeparator();
    QAction* quitAction = trayMenu->addAction("Quitter");

    connect(listAction, &QAction::triggered, this, &NotesManager::onShowNotesListRequested);
    connect(newAction, &QAction::triggered, this, &NotesManager::onNewNoteRequested);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(trayMenu);
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &NotesManager::onTrayIconActivated);

    m_trayIcon->show();
}

void NotesManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onShowNotesListRequested();
    }
}

void NotesManager::syncWithServer() {
    if (!m_sync || !m_sync->isEnabled()) return;

    m_sync->pullNotes([this](const std::vector<NoteModel>& remoteNotes, bool success) {
        if (!success) return;

        // 1. Filtrer les notes distantes pour retirer celles qui ont été supprimées localement et sont en attente de synchro
        std::vector<NoteModel> filteredRemote;
        for (const auto& rNote : remoteNotes) {
            if (!m_deletedNotesPendingSync.contains(rNote.id)) {
                filteredRemote.push_back(rNote);
            }
        }

        // Fusionner les notes locales et distantes filtrées
        std::vector<NoteModel> merged = m_sync->mergeNotes(m_notes, filteredRemote);

        bool hasLocalChanges = false;
        bool hasRemoteChanges = false;

        // 2. Détecter s'il y a des changements provenant du serveur
        if (merged.size() != m_notes.size()) {
            hasRemoteChanges = true;
        } else {
            for (size_t i = 0; i < m_notes.size(); ++i) {
                if (m_notes[i] != merged[i]) {
                    hasRemoteChanges = true;
                    break;
                }
            }
        }

        // 3. Détecter s'il y a des modifications locales à pousser (en comparant avec les notes d'origine du serveur)
        if (merged.size() != remoteNotes.size()) {
            hasLocalChanges = true;
        } else {
            for (const auto& mNote : merged) {
                auto rIt = std::find_if(remoteNotes.begin(), remoteNotes.end(), [&mNote](const NoteModel& rNote) {
                    return rNote.id == mNote.id;
                });
                if (rIt == remoteNotes.end() || mNote != *rIt || !mNote.synced) {
                    hasLocalChanges = true;
                    break;
                }
            }
        }

        if (hasRemoteChanges) {
            m_notes = merged;
            m_storage->saveNotes(m_notes);
            updateWindowsFromNotes();
            if (m_listWindow) {
                m_listWindow->updateNotesList(m_notes);
            }
        }

        if (hasLocalChanges) {
            m_sync->pushNotes(m_notes);
        } else {
            // Si pas de changements locaux à pousser, on peut nettoyer la liste des suppressions en attente
            m_deletedNotesPendingSync.clear();
        }
    });
}

void NotesManager::updateWindowsFromNotes() {
    // 1. Fermer ou mettre à jour les fenêtres ouvertes
    for (auto it = m_windows.begin(); it != m_windows.end(); ) {
        QString id = it->first;
        StickyWindow* win = it->second;

        auto noteIt = std::find_if(m_notes.begin(), m_notes.end(), [&id](const NoteModel& n) {
            return n.id == id;
        });

        if (noteIt == m_notes.end() || noteIt->archived || noteIt->trashed) {
            win->close();
            delete win;
            it = m_windows.erase(it);
        } else {
            win->updateFromModel(*noteIt);
            ++it;
        }
    }

    // 2. Créer des fenêtres pour les nouvelles notes distantes
    for (const auto& note : m_notes) {
        if (!note.archived && !note.trashed) {
            if (m_windows.find(note.id) == m_windows.end()) {
                createNoteWindow(note);
            }
        }
    }
}
