#ifndef NOTESLISTWINDOW_H
#define NOTESLISTWINDOW_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include <QMouseEvent>
#include "notemodel.h"

// Widget personnalisé représentant une carte Post-it moderne (Mode Grille Bento ou Liste)
class NoteCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit NoteCardWidget(const NoteModel& note, bool isGridView, bool darkTheme, QWidget* parent = nullptr);
    QString id() const { return m_id; }
    QString color() const { return m_color; }
    const NoteModel& noteModel() const { return m_note; }

signals:
    void openRequested(const QString& id);
    void deleteRequested(const QString& id);

private:
    QString getTitleFromContent(const QString& plainText) const;
    QString getPlainPreview(const QString& htmlContent) const;
    QString getFormattedDate(const QDateTime& dt) const;

    NoteModel m_note;
    QString m_id;
    QString m_color;
    bool m_isGridView;
    QPushButton* m_openButton;
    QPushButton* m_deleteButton;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};

// Fenêtre de tableau de bord de tous les Post-its (extensible et moderne)
class NotesListWindow : public QWidget {
    Q_OBJECT
public:
    explicit NotesListWindow(const std::vector<NoteModel>& notes, QWidget *parent = nullptr);
    ~NotesListWindow() override;

    void updateNotesList(const std::vector<NoteModel>& notes);

signals:
    void openNoteRequested(const QString& id);
    void deleteNoteRequested(const QString& id);
    void newNoteRequested();
    void mobileSyncRequested();
    void checkUpdateRequested();
    void applyUpdateRequested();

public slots:
    void setUpdateAvailable(bool available, const QString& currentHash = "", const QString& remoteHash = "", const QString& commitSubject = "");
    void setUpdateChecking(bool checking);

private slots:
    void filterNotes();
    void toggleTheme();
    void toggleViewMode();
    void onColorFilterClicked(const QString& colorHex);
    void onUpdateBtnClicked();
    void toggleAutostart();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUi();
    void applyTheme(bool dark);
    bool loadThemeSetting();
    void saveThemeSetting(bool dark);
    bool loadViewModeSetting();
    void saveViewModeSetting(bool isGrid);
    void populateList();
    void updateSyncBadge();
    void updateAutostartButton();

    QListWidget* m_listWidget;
    QLineEdit* m_searchEdit;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_syncBadge;
    QPushButton* m_viewToggleBtn;
    QPushButton* m_themeToggleBtn;
    QPushButton* m_mobileSyncBtn;
    QPushButton* m_autostartBtn;
    QPushButton* m_updateBtn;
    QPushButton* m_newNoteBtn;
    QPushButton* m_closeButton;
    QHBoxLayout* m_filtersLayout;
    QList<QPushButton*> m_filterChips;

    std::vector<NoteModel> m_notes;
    bool m_darkTheme;
    bool m_isGridView = true;
    QString m_selectedColorFilter = "";
    bool m_hasUpdate = false;
    QString m_currentHash;
    QString m_remoteHash;
    QString m_commitSubject;
};

#endif // NOTESLISTWINDOW_H
