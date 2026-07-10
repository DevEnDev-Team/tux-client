#ifndef STICKYWINDOW_H
#define STICKYWINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPoint>
#include <QTimer>
#include "notemodel.h"

class StickyWindow : public QWidget {
    Q_OBJECT

public:
    explicit StickyWindow(NoteModel model, QWidget *parent = nullptr);
    ~StickyWindow();

    NoteModel getModel() const;
    void updateAppearance();
    void setSynced(bool synced);
    void updateFromModel(const NoteModel& model);
    static QIcon createPenguinIcon();
    static QIcon createLockIcon(bool locked);
    static QIcon createPlusIcon();
    static QIcon createColorBadgeIcon(const QString& hexColor);
    static QIcon createBoldIcon();
    static QIcon createSyncIcon(bool synced, bool enabled);

signals:
    void noteChanged(const QString& id);
    void newNoteRequested();
    void duplicateNoteRequested(const NoteModel& model);
    void deleteNoteRequested(const QString& id);
    void showSearchRequested();
    void showNotesListRequested();
    void logoutRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onContentChanged();
    void saveTriggered();
    void showContextMenu(const QPoint& pos);
    void changeColor(const QString& hexColor);
    void chooseCustomColor();
    void toggleAlwaysOnTop();
    void toggleLocked();
    void toggleAutostart();
    void setOpacityValue(double opacity);
    void deleteNote();

private:
    void setupUi();
    void setupShortcuts();
    QColor getContrastColor(const QColor& background) const;
    void adjustFontSize(int delta);
    void updateTitle();
    void convertPlainListToHtmlList();
    static QIcon createFormatIcon();

    NoteModel m_model;
    
    // UI Elements
    QWidget* m_headerWidget;
    QPushButton* m_menuButton;
    QLabel* m_titleLabel;
    QPushButton* m_plusButton;
    QPushButton* m_formatButton;
    QPushButton* m_colorBadgeButton;
    QPushButton* m_lockButton;
    QPushButton* m_syncButton;
    QPushButton* m_closeButton;
    QTextEdit* m_textEdit;

    // Window dragging & resizing helpers for borderless window
    QPoint m_dragPosition;
    bool m_dragActive = false;
    bool m_resizeActive = false;
    int m_resizeEdge = 0; // 1: bottom-right, etc.
    const int m_resizeMargin = 20;

    QTimer* m_autosaveTimer;
    int m_currentFontSize = 12;
};

#endif // STICKYWINDOW_H
