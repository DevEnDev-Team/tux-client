#include "stickywindow.h"
#include <QMouseEvent>
#include <QMenu>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QStyle>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QDateTime>
#include <QWindow>
#include <QStyleOption>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextDocument>

StickyWindow::StickyWindow(NoteModel model, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::CustomizeWindowHint)
    , m_model(model)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(180, 180);
    resize(m_model.size);
    move(m_model.position);

    setupUi();
    updateAppearance();

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setSingleShot(true);
    connect(m_autosaveTimer, &QTimer::timeout, this, &StickyWindow::saveTriggered);

    connect(m_textEdit, &QTextEdit::textChanged, this, &StickyWindow::onContentChanged);

    // Apply shadow if borderless
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);
}

StickyWindow::~StickyWindow() {}

NoteModel StickyWindow::getModel() const {
    NoteModel m = m_model;
    m.position = pos();
    m.size = size();
    m.content = m_textEdit->toPlainText();
    return m;
}

void StickyWindow::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Header layout
    m_headerWidget = new QWidget(this);
    m_headerWidget->setObjectName("headerWidget");
    m_headerWidget->setFixedHeight(32);
    QHBoxLayout* headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(8, 0, 8, 0);
    headerLayout->setSpacing(4);
    headerLayout->setAlignment(Qt::AlignVCenter);

    m_menuButton = new QPushButton(m_headerWidget);
    m_menuButton->setIcon(createPenguinIcon());
    m_menuButton->setIconSize(QSize(20, 20));
    m_menuButton->setCursor(Qt::PointingHandCursor);
    m_menuButton->setFixedSize(26, 26);
    m_menuButton->setFlat(true);

    m_titleLabel = new QLabel(m_model.tags.isEmpty() ? "Tux-It" : m_model.tags.join(", "), m_headerWidget);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    m_plusButton = new QPushButton(m_headerWidget);
    m_plusButton->setIconSize(QSize(20, 20));
    m_plusButton->setCursor(Qt::PointingHandCursor);
    m_plusButton->setFixedSize(26, 26);
    m_plusButton->setFlat(true);

    m_boldButton = new QPushButton(m_headerWidget);
    m_boldButton->setIconSize(QSize(20, 20));
    m_boldButton->setCursor(Qt::PointingHandCursor);
    m_boldButton->setFixedSize(26, 26);
    m_boldButton->setFlat(true);

    m_colorBadgeButton = new QPushButton(m_headerWidget);
    m_colorBadgeButton->setIconSize(QSize(20, 20));
    m_colorBadgeButton->setCursor(Qt::PointingHandCursor);
    m_colorBadgeButton->setFixedSize(26, 26);
    m_colorBadgeButton->setFlat(true);

    m_lockButton = new QPushButton(m_headerWidget);
    m_lockButton->setIconSize(QSize(14, 14));
    m_lockButton->setCursor(Qt::PointingHandCursor);
    m_lockButton->setFixedSize(26, 26);
    m_lockButton->setFlat(true);

    m_syncButton = new QPushButton(m_headerWidget);
    m_syncButton->setIconSize(QSize(20, 20));
    m_syncButton->setCursor(Qt::ArrowCursor);
    m_syncButton->setFixedSize(26, 26);
    m_syncButton->setFlat(true);

    m_closeButton = new QPushButton(m_headerWidget);
    m_closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    m_closeButton->setIconSize(QSize(16, 16));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFixedSize(26, 26);
    m_closeButton->setFlat(true);

    headerLayout->addWidget(m_menuButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
    headerLayout->addStretch();
    headerLayout->addWidget(m_plusButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_boldButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_colorBadgeButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_syncButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_lockButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);

    // Text Editor
    m_textEdit = new QTextEdit(this);
    m_textEdit->setFrameStyle(QFrame::NoFrame);
    m_textEdit->setAcceptRichText(true);
    m_textEdit->setHtml(m_model.content);
    m_textEdit->setPlaceholderText("Écrivez quelque chose...");
    
    // Set padding via stylesheet or margins
    m_textEdit->document()->setDocumentMargin(12);

    mainLayout->addWidget(m_headerWidget);
    mainLayout->addWidget(m_textEdit);

    m_headerWidget->installEventFilter(this);
    m_titleLabel->installEventFilter(this);
    m_menuButton->installEventFilter(this);
    m_textEdit->installEventFilter(this);
    m_textEdit->viewport()->installEventFilter(this);
    m_textEdit->setMouseTracking(true);
    m_textEdit->viewport()->setMouseTracking(true);
    setMouseTracking(true);

    // Connections
    connect(m_closeButton, &QPushButton::clicked, this, &StickyWindow::deleteNote);
    connect(m_lockButton, &QPushButton::clicked, this, &StickyWindow::toggleLocked);
    connect(m_plusButton, &QPushButton::clicked, this, &StickyWindow::newNoteRequested);
    connect(m_boldButton, &QPushButton::clicked, this, [this]() {
        QTextCharFormat format;
        format.setFontWeight(m_textEdit->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        m_textEdit->mergeCurrentCharFormat(format);
        saveTriggered();
    });
    connect(m_colorBadgeButton, &QPushButton::clicked, this, [this]() {
        QMenu colorMenu(this);
        colorMenu.setAttribute(Qt::WA_TranslucentBackground);
        colorMenu.setStyleSheet(
            "QMenu { background-color: palette(window); color: palette(text); border: 1px solid palette(mid); border-radius: 8px; padding: 4px; }"
            "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
            "QMenu::item:selected { background-color: palette(highlight); color: palette(highlightedText); }"
            "QMenu::separator { height: 1px; background: palette(mid); margin: 4px 0; }"
        );

        colorMenu.addAction("Jaune")->setData("#FFF59D");
        colorMenu.addAction("Bleu")->setData("#90CAF9");
        colorMenu.addAction("Vert")->setData("#A5D6A7");
        colorMenu.addAction("Rose")->setData("#F8BBD0");
        colorMenu.addAction("Orange")->setData("#FFCC80");
        colorMenu.addAction("Violet")->setData("#CE93D8");
        colorMenu.addAction("Gris")->setData("#E0E0E0");
        colorMenu.addAction("Blanc")->setData("#FAFAFA");
        colorMenu.addAction("Personnalisée...");

        QPoint spawnPos = m_colorBadgeButton->mapToGlobal(QPoint(0, m_colorBadgeButton->height()));
        QAction* selected = colorMenu.exec(spawnPos);
        if (!selected) return;

        if (selected->text() == "Personnalisée...") {
            chooseCustomColor();
        } else {
            changeColor(selected->data().toString());
        }
    });
    
    // Setup Context Menu for Menu Button
    connect(m_menuButton, &QPushButton::clicked, this, [this]() {
        QPoint globalPos = m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height()));
        QPoint localToText = m_textEdit->mapFromGlobal(globalPos);
        showContextMenu(localToText);
    });
    
    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit, &QWidget::customContextMenuRequested, this, &StickyWindow::showContextMenu);
    updateTitle();
}

void StickyWindow::updateAppearance() {
    // Set Window Flags
    Qt::WindowFlags flags = Qt::Window | Qt::FramelessWindowHint | Qt::CustomizeWindowHint;
    if (m_model.alwaysOnTop) {
        flags |= Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint;
    }
    
    if (windowFlags() != flags) {
        setWindowFlags(flags);
        show();
    }

    // Set opacity after flags are set
    m_model.opacity = 1.0;
    setWindowOpacity(1.0);

    QColor bgColor(m_model.color);
    QColor fgColor = getContrastColor(bgColor);

    // Colors & Stylesheets
    QString style = QString(
        "StickyWindow { background-color: %1; border-radius: 12px; }"
        "QWidget#headerWidget { background-color: transparent; }"
        "QPushButton { border: none; background: transparent; color: %2; font-size: 14px; }"
        "QPushButton::hover { background-color: rgba(0, 0, 0, 0.1); border-radius: 4px; }"
        "QTextEdit { background-color: transparent; border: none; font-size: %3px; color: %2; }"
    ).arg(bgColor.name(), fgColor.name()).arg(m_currentFontSize);

    setStyleSheet(style);

    m_titleLabel->setStyleSheet(QString("font-weight: bold; font-size: 13px; color: %1;").arg(fgColor.name()));
    m_plusButton->setIcon(createPlusIcon());
    m_boldButton->setIcon(createBoldIcon());
    m_colorBadgeButton->setIcon(createColorBadgeIcon(m_model.color));
    m_lockButton->setIcon(createLockIcon(m_model.locked));

    // Détecter si la synchro est configurée localement en lisant la config
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";
    bool syncConfigured = false;
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        syncConfigured = !obj.value("server_url").toString().trimmed().isEmpty() && 
                         !obj.value("api_key").toString().trimmed().isEmpty();
        file.close();
    }

    m_syncButton->setVisible(syncConfigured); // Cacher si non configurée
    if (syncConfigured) {
        m_syncButton->setIcon(createSyncIcon(m_model.synced, true));
        m_syncButton->setToolTip(m_model.synced ? "Note synchronisée en ligne" : "Modifications locales non synchronisées");
    }
    
    m_textEdit->setReadOnly(m_model.locked);
}

void StickyWindow::updateTitle() {
    QTextDocument doc;
    doc.setHtml(m_textEdit ? m_textEdit->toHtml() : m_model.content);
    QString plainText = doc.toPlainText().trimmed();
    
    QString title = "Tux-It";
    if (!plainText.isEmpty()) {
        int index = -1;
        for (int i = 0; i < plainText.length(); ++i) {
            QChar ch = plainText.at(i);
            if (ch == '.' || ch == '!' || ch == '?' || ch == '\n') {
                index = i;
                break;
            }
        }
        QString firstSentence = (index == -1) ? plainText : plainText.left(index);
        firstSentence = firstSentence.trimmed();
        
        if (firstSentence.isEmpty()) {
            firstSentence = plainText;
        }

        QStringList rawWords = firstSentence.split(' ');
        QStringList cleanWords;
        for (const QString& w : rawWords) {
            QString trimmedW = w.trimmed();
            if (!trimmedW.isEmpty()) {
                cleanWords.append(trimmedW);
            }
        }
        
        if (!cleanWords.isEmpty()) {
            int wordsCount = qMin(3, cleanWords.size());
            QStringList titleWords;
            for (int i = 0; i < wordsCount; ++i) {
                titleWords.append(cleanWords.at(i));
            }
            title = titleWords.join(" ");
        }
    }
    
    QString finalTitle = m_model.tags.isEmpty() ? title : m_model.tags.join(", ");
    if (m_titleLabel) {
        m_titleLabel->setText(finalTitle);
    }
}

QColor StickyWindow::getContrastColor(const QColor& background) const {
    // YIQ formula to decide black or white text
    int yiq = ((background.red() * 299) + (background.green() * 587) + (background.blue() * 114)) / 1000;
    return (yiq >= 128) ? QColor("#212121") : QColor("#FAFAFA");
}

void StickyWindow::onContentChanged() {
    m_model.lastModified = QDateTime::currentDateTime();
    updateTitle();
    m_autosaveTimer->start(500); // 500 ms autosave debounce
}

void StickyWindow::saveTriggered() {
    m_model.content = m_textEdit->toHtml();
    m_model.position = pos();
    m_model.size = size();
    m_model.lastModified = QDateTime::currentDateTime();
    updateTitle();
    emit noteChanged(m_model.id);
}

void StickyWindow::deleteNote() {
    emit deleteNoteRequested(m_model.id);
}

void StickyWindow::toggleAlwaysOnTop() {
    m_model.alwaysOnTop = !m_model.alwaysOnTop;
    updateAppearance();
    saveTriggered();
}

void StickyWindow::toggleLocked() {
    m_model.locked = !m_model.locked;
    updateAppearance();
    saveTriggered();
}

void StickyWindow::setOpacityValue(double opacity) {
    m_model.opacity = opacity;
    updateAppearance();
    saveTriggered();
}

void StickyWindow::changeColor(const QString& hexColor) {
    m_model.color = hexColor;
    updateAppearance();
    saveTriggered();
}

void StickyWindow::chooseCustomColor() {
    QColor col = QColorDialog::getColor(QColor(m_model.color), this, "Choisir une couleur");
    if (col.isValid()) {
        changeColor(col.name());
    }
}

void StickyWindow::showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setStyleSheet(
        "QMenu { background-color: palette(window); color: palette(text); border: 1px solid palette(mid); border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: palette(highlight); color: palette(highlightedText); }"
        "QMenu::separator { height: 1px; background: palette(mid); margin: 4px 0; }"
    );

    QAction* actionNew = menu.addAction("Nouvelle note");
    QAction* actionDuplicate = menu.addAction("Dupliquer");
    QAction* actionList = menu.addAction("Liste des notes");
    menu.addSeparator();

    QMenu* colorMenu = menu.addMenu("Changer couleur");
    colorMenu->addAction("Jaune")->setData("#FFF59D");
    colorMenu->addAction("Bleu")->setData("#90CAF9");
    colorMenu->addAction("Vert")->setData("#A5D6A7");
    colorMenu->addAction("Rose")->setData("#F8BBD0");
    colorMenu->addAction("Orange")->setData("#FFCC80");
    colorMenu->addAction("Violet")->setData("#CE93D8");
    colorMenu->addAction("Gris")->setData("#E0E0E0");
    colorMenu->addAction("Blanc")->setData("#FAFAFA");
    colorMenu->addAction("Personnalisée...");

    QAction* actionPin = menu.addAction("Toujours devant");
    actionPin->setCheckable(true);
    actionPin->setChecked(m_model.alwaysOnTop);

    QAction* actionLock = menu.addAction("Verrouiller");
    actionLock->setCheckable(true);
    actionLock->setChecked(m_model.locked);

    bool autostartEnabled = QFile::exists(QDir::homePath() + "/.config/autostart/tux-it.desktop");
    QAction* actionAutostart = menu.addAction("Lancer au démarrage");
    actionAutostart->setCheckable(true);
    actionAutostart->setChecked(autostartEnabled);

    // Détecter si la synchro est active
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";
    bool syncActive = false;
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        syncActive = !obj.value("server_url").toString().trimmed().isEmpty() && 
                     !obj.value("api_key").toString().trimmed().isEmpty();
        file.close();
    }

    QAction* actionLogout = nullptr;
    if (syncActive) {
        menu.addSeparator();
        actionLogout = menu.addAction("Se déconnecter (Mode local)");
    }

    menu.addSeparator();
    QAction* actionDelete = menu.addAction("Supprimer");

    // Execution
    QAction* selected = menu.exec(m_textEdit->mapToGlobal(pos));
    if (!selected) return;

    if (selected == actionNew) {
        emit newNoteRequested();
    } else if (selected == actionDuplicate) {
        emit duplicateNoteRequested(getModel());
    } else if (selected == actionList) {
        emit showNotesListRequested();
    } else if (selected == actionPin) {
        toggleAlwaysOnTop();
    } else if (selected == actionLock) {
        toggleLocked();
    } else if (selected == actionAutostart) {
        toggleAutostart();
    } else if (actionLogout && selected == actionLogout) {
        emit logoutRequested();
    } else if (selected == actionDelete) {
        deleteNote();
    } else if (selected->parent() == colorMenu) {
        if (selected->text() == "Personnalisée...") {
            chooseCustomColor();
        } else {
            changeColor(selected->data().toString());
        }
    }
}

void StickyWindow::mousePressEvent(QMouseEvent *event) {
    if (m_model.locked) return;

    if (event->button() == Qt::LeftButton) {
        int xPos = event->position().x();
        int yPos = event->position().y();
        
        if (xPos >= width() - m_resizeMargin && yPos >= height() - m_resizeMargin) {
            m_resizeActive = true;
            m_resizeEdge = 3;
            setCursor(Qt::SizeFDiagCursor);
        } else if (xPos >= width() - m_resizeMargin) {
            m_resizeActive = true;
            m_resizeEdge = 1;
            setCursor(Qt::SizeHorCursor);
        } else if (yPos >= height() - m_resizeMargin) {
            m_resizeActive = true;
            m_resizeEdge = 2;
            setCursor(Qt::SizeVerCursor);
        } else {
            m_dragActive = true;
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
    }
}

void StickyWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_model.locked) return;

    // Changer le curseur au survol sans clic
    if (!(event->buttons() & Qt::LeftButton)) {
        int xPos = event->position().x();
        int yPos = event->position().y();
        if (xPos >= width() - m_resizeMargin && yPos >= height() - m_resizeMargin) {
            setCursor(Qt::SizeFDiagCursor);
        } else if (xPos >= width() - m_resizeMargin) {
            setCursor(Qt::SizeHorCursor);
        } else if (yPos >= height() - m_resizeMargin) {
            setCursor(Qt::SizeVerCursor);
        } else {
            unsetCursor();
        }
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        if (m_resizeActive) {
            QPoint globalPos = event->globalPosition().toPoint();
            int newWidth = width();
            int newHeight = height();
            if (m_resizeEdge == 1 || m_resizeEdge == 3) {
                newWidth = qMax(minimumWidth(), globalPos.x() - x());
            }
            if (m_resizeEdge == 2 || m_resizeEdge == 3) {
                newHeight = qMax(minimumHeight(), globalPos.y() - y());
            }
            resize(newWidth, newHeight);
            saveTriggered();
        } else if (m_dragActive) {
            move(event->globalPosition().toPoint() - m_dragPosition);
            saveTriggered();
        }
        event->accept();
    }
}

void StickyWindow::mouseReleaseEvent(QMouseEvent *event) {
    m_resizeActive = false;
    m_resizeEdge = 0;
    m_dragActive = false;
    unsetCursor();
    QWidget::mouseReleaseEvent(event);
}

void StickyWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void StickyWindow::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_N:
                emit newNoteRequested();
                event->accept();
                return;
            case Qt::Key_L:
                toggleLocked();
                event->accept();
                return;
            case Qt::Key_Plus:
            case Qt::Key_Equal:
                adjustFontSize(1);
                event->accept();
                return;
            case Qt::Key_Minus:
                adjustFontSize(-1);
                event->accept();
                return;
            default:
                break;
        }
    } else if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (event->key() == Qt::Key_P) {
            toggleAlwaysOnTop();
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void StickyWindow::adjustFontSize(int delta) {
    m_currentFontSize = qBound(8, m_currentFontSize + delta, 36);
    updateAppearance();
}

bool StickyWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_menuButton) {
        if (event->type() == QEvent::Enter) {
            m_menuButton->setIconSize(QSize(24, 24));
            return true;
        } else if (event->type() == QEvent::Leave) {
            m_menuButton->setIconSize(QSize(20, 20));
            return true;
        }
    }

    if ((watched == m_headerWidget || watched == m_titleLabel) && !m_model.locked) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // Vérifier si le clic est sur un bouton du header
                QPoint localPos = m_headerWidget->mapFromGlobal(mouseEvent->globalPosition().toPoint());
                QWidget* child = m_headerWidget->childAt(localPos);
                if (qobject_cast<QPushButton*>(child)) {
                    return false; // Laisse le bouton gérer le clic
                }
                
                // Utiliser le déplacement système natif (X11/Wayland)
                if (windowHandle()) {
                    windowHandle()->startSystemMove();
                    return true;
                }
            }
        }
    }

    if ((watched == m_textEdit || watched == m_textEdit->viewport()) && !m_model.locked) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());
            
            // Si redimensionnement actif, changer les dimensions
            if (m_resizeActive) {
                QPoint globalPos = mouseEvent->globalPosition().toPoint();
                int newWidth = width();
                int newHeight = height();
                if (m_resizeEdge == 1 || m_resizeEdge == 3) {
                    newWidth = qMax(minimumWidth(), globalPos.x() - x());
                }
                if (m_resizeEdge == 2 || m_resizeEdge == 3) {
                    newHeight = qMax(minimumHeight(), globalPos.y() - y());
                }
                resize(newWidth, newHeight);
                saveTriggered();
                return true;
            }

            // Changer le curseur au survol
            if (localPos.x() >= width() - m_resizeMargin && localPos.y() >= height() - m_resizeMargin) {
                m_textEdit->viewport()->setCursor(Qt::SizeFDiagCursor);
            } else if (localPos.x() >= width() - m_resizeMargin) {
                m_textEdit->viewport()->setCursor(Qt::SizeHorCursor);
            } else if (localPos.y() >= height() - m_resizeMargin) {
                m_textEdit->viewport()->setCursor(Qt::SizeVerCursor);
            } else {
                m_textEdit->viewport()->setCursor(Qt::IBeamCursor);
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());
            
            if (mouseEvent->button() == Qt::LeftButton) {
                if (localPos.x() >= width() - m_resizeMargin && localPos.y() >= height() - m_resizeMargin) {
                    m_resizeActive = true;
                    m_resizeEdge = 3;
                    return true; // Ne pas propager le clic au QTextEdit
                } else if (localPos.x() >= width() - m_resizeMargin) {
                    m_resizeActive = true;
                    m_resizeEdge = 1;
                    return true;
                } else if (localPos.y() >= height() - m_resizeMargin) {
                    m_resizeActive = true;
                    m_resizeEdge = 2;
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_resizeActive = false;
            m_resizeEdge = 0;
            m_textEdit->viewport()->setCursor(Qt::IBeamCursor);
        }
    }

    return QWidget::eventFilter(watched, event);
}

void StickyWindow::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    
    // Dessiner une poignée de redimensionnement discrète dans le coin inférieur droit
    if (!m_model.locked) {
        p.setRenderHint(QPainter::Antialiasing);
        QColor handleColor = getContrastColor(QColor(m_model.color));
        handleColor.setAlpha(60); // Translucide et subtil
        p.setPen(QPen(handleColor, 1.5, Qt::SolidLine, Qt::RoundCap));

        int w = width();
        int h = height();
        // Dessiner trois petites lignes diagonales
        p.drawLine(w - 12, h - 4, w - 4, h - 12);
        p.drawLine(w - 8, h - 4, w - 4, h - 8);
        p.drawLine(w - 16, h - 4, w - 4, h - 16);
    }

    QWidget::paintEvent(event);
}

QIcon StickyWindow::createPenguinIcon() {
    return QIcon(":/logo.png");
}

QIcon StickyWindow::createLockIcon(bool locked) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bodyColor("#757575");
    QColor shackleColor("#9E9E9E");

    // Dessiner l'anse (shackle)
    painter.setPen(QPen(shackleColor, 2.5, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    
    if (locked) {
        // Anse fermée : un arc
        painter.drawArc(8, 6, 16, 16, 0, 180 * 16);
        // Lignes verticales qui descendent dans le corps
        painter.drawLine(8, 14, 8, 18);
        painter.drawLine(24, 14, 24, 18);
    } else {
        // Anse ouverte : décalée vers le haut et sur le côté
        painter.drawArc(8, 2, 16, 16, 0, 180 * 16);
        painter.drawLine(8, 10, 8, 18);
        painter.drawLine(24, 10, 24, 12);
    }

    // Dessiner le corps du cadenas (rectangle)
    painter.setPen(Qt::NoPen);
    painter.setBrush(bodyColor);
    painter.drawRoundedRect(5, 16, 22, 12, 2, 2);

    // Dessiner le trou de serrure
    painter.setBrush(QColor("#212121"));
    painter.drawEllipse(14, 20, 4, 4);

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createPlusIcon() {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color("#757575");
    painter.setPen(QPen(color, 3, Qt::SolidLine, Qt::RoundCap));
    
    // Ligne horizontale
    painter.drawLine(8, 16, 24, 16);
    // Ligne verticale
    painter.drawLine(16, 8, 16, 24);

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createColorBadgeIcon(const QString& hexColor) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color(hexColor);
    painter.setBrush(color);
    painter.setPen(QPen(QColor("#757575"), 1.5));
    painter.drawEllipse(6, 6, 20, 20);

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createBoldIcon() {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color("#757575");
    QFont font = painter.font();
    font.setPointSize(14);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, "B");

    painter.end();
    return QIcon(pixmap);
}

void StickyWindow::toggleAutostart() {
    QString homePath = QDir::homePath();
    QString autostartDir = homePath + "/.config/autostart";
    QString autostartPath = autostartDir + "/tux-it.desktop";

    if (QFile::exists(autostartPath)) {
        QFile::remove(autostartPath);
    } else {
        QDir().mkpath(autostartDir);
        QFile file(autostartPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Name=Tux-It\n"
                << "Comment=Vos post-its mignons sur le bureau\n"
                << "Exec=" << homePath << "/.local/bin/tux-it\n"
                << "Icon=" << homePath << "/.local/share/icons/tux-it.png\n"
                << "Terminal=false\n"
                << "Type=Application\n"
                << "Categories=Utility;Office;\n"
                << "StartupNotify=true\n";
            file.close();
            QFile::setPermissions(autostartPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                                                 QFile::ReadGroup | QFile::ReadOther);
        }
    }
}

void StickyWindow::setSynced(bool synced) {
    m_model.synced = synced;
    updateAppearance();
}

void StickyWindow::updateFromModel(const NoteModel& model) {
    // Si le contenu a changé, on le met à jour
    if (m_model.content != model.content) {
        // Sauvegarder la position du curseur
        QTextCursor cursor = m_textEdit->textCursor();
        int position = cursor.position();
        
        m_model.content = model.content;
        m_textEdit->setHtml(model.content);
        
        // Restaurer la position du curseur
        cursor.setPosition(qMin(position, m_textEdit->toPlainText().length()));
        m_textEdit->setTextCursor(cursor);
    }
    
    // Si la position ou la taille diffèrent (et qu'on ne fait pas de drag/resize en ce moment)
    if (!m_dragActive && !m_resizeActive) {
        if (pos() != model.position) {
            move(model.position);
        }
        if (size() != model.size) {
            resize(model.size);
        }
    }
    
    m_model.color = model.color;
    m_model.alwaysOnTop = model.alwaysOnTop;
    m_model.locked = model.locked;
    m_model.synced = model.synced;
    m_model.lastModified = model.lastModified;
    m_model.tags = model.tags;
    
    updateAppearance();
    updateTitle();
}


QIcon StickyWindow::createSyncIcon(bool synced, bool enabled) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!enabled) {
        painter.end();
        return QIcon();
    }

    // Définir la couleur du nuage selon le statut de synchro
    QColor cloudColor = synced ? QColor("#4CAF50") : QColor("#FFA726"); // Vert si synchro, Orange si non sync

    // Dessiner un nuage vectoriel mimi
    painter.setPen(Qt::NoPen);
    painter.setBrush(cloudColor);

    // Dessiner le corps du nuage (plusieurs cercles se chevauchant)
    painter.drawEllipse(12, 6, 10, 10); // Milieu haut
    painter.drawEllipse(6, 10, 8, 8);    // Gauche
    painter.drawEllipse(18, 10, 8, 8);   // Droite
    painter.drawRect(10, 12, 12, 6);     // Base plate

    // Si synchronisé (synced = true), dessiner un petit check de validation blanc en surimpression
    if (synced) {
        painter.setPen(QPen(Qt::white, 2));
        painter.drawLine(12, 13, 14, 15);
        painter.drawLine(14, 15, 19, 10);
    } else {
        // Si non synchronisé (synced = false), dessiner une ligne d'exclamation blanche
        painter.setPen(QPen(Qt::white, 2));
        painter.drawLine(16, 10, 16, 13);
        painter.drawPoint(16, 15);
    }

    painter.end();
    return QIcon(pixmap);
}


