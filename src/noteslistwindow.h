#ifndef NOTESLISTWINDOW_H
#define NOTESLISTWINDOW_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include "notemodel.h"

// Widget personnalisé représentant un Post-it miniature sous forme de carte cool
class NoteCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit NoteCardWidget(const NoteModel& note, QWidget* parent = nullptr);
    QString id() const { return m_id; }

signals:
    void openRequested(const QString& id);
    void deleteRequested(const QString& id);

private:
    QString getTitleFromContent(const QString& htmlContent) const;
    QString getPlainPreview(const QString& htmlContent) const;

    QString m_id;
    QPushButton* m_openButton;
    QPushButton* m_deleteButton;

protected:
    void paintEvent(QPaintEvent* event) override;
};

// Fenêtre de liste de tous les Post-its
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

private slots:
    void filterNotes(const QString& text);
    void toggleTheme();

private:
    void setupUi();
    void applyTheme(bool dark);
    bool loadThemeSetting();
    void saveThemeSetting(bool dark);

    QListWidget* m_listWidget;
    QLineEdit* m_searchEdit;
    QPushButton* m_closeButton;
    QPushButton* m_themeToggleBtn;
    std::vector<NoteModel> m_notes;
    bool m_darkTheme;
};

#endif // NOTESLISTWINDOW_H
