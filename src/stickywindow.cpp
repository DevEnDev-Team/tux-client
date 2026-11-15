#include "stickywindow.h"
#include "autostartmanager.h"
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
#include <QPainterPath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextList>
#include <QRegularExpression>

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

    // Apply shadow if borderless (effet physique doux 2026)
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(24);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    setGraphicsEffect(shadow);
}

StickyWindow::~StickyWindow() {}

NoteModel StickyWindow::getModel() const {
    NoteModel m = m_model;
    m.position = pos();
    m.size = size();
    m.content = m_textEdit->toHtml();
    return m;
}

void StickyWindow::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Header layout moderne (36px de hauteur)
    m_headerWidget = new QWidget(this);
    m_headerWidget->setObjectName("headerWidget");
    m_headerWidget->setFixedHeight(36);
    QHBoxLayout* headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(10, 0, 10, 0);
    headerLayout->setSpacing(4);
    headerLayout->setAlignment(Qt::AlignVCenter);

    m_menuButton = new QPushButton(m_headerWidget);
    m_menuButton->setIcon(createPenguinIcon());
    m_menuButton->setIconSize(QSize(16, 16));
    m_menuButton->setCursor(Qt::PointingHandCursor);
    m_menuButton->setFixedSize(26, 26);
    m_menuButton->setToolTip("Menu principal Tux-It");

    m_titleLabel = new QLabel(m_model.tags.isEmpty() ? "TUX-IT" : m_model.tags.join(", "), m_headerWidget);
    m_titleLabel->setStyleSheet("font-weight: 700; font-size: 11px; letter-spacing: 0.5px; opacity: 0.75;");

    m_pinButton = new QPushButton(m_headerWidget);
    m_pinButton->setCursor(Qt::PointingHandCursor);
    m_pinButton->setFixedSize(26, 26);
    m_pinButton->setToolTip(m_model.alwaysOnTop ? "Désépingler du premier plan" : "Toujours au premier plan");

    m_colorBadgeButton = new QPushButton(m_headerWidget);
    m_colorBadgeButton->setCursor(Qt::PointingHandCursor);
    m_colorBadgeButton->setFixedSize(26, 26);
    m_colorBadgeButton->setToolTip("Changer de couleur");

    m_plusButton = new QPushButton(m_headerWidget);
    m_plusButton->setCursor(Qt::PointingHandCursor);
    m_plusButton->setFixedSize(26, 26);
    m_plusButton->setToolTip("Nouvelle note (Ctrl+N)");

    m_formatButton = new QPushButton(m_headerWidget);
    m_formatButton->setCursor(Qt::PointingHandCursor);
    m_formatButton->setFixedSize(26, 26);
    m_formatButton->setToolTip("Formatage du texte");

    m_lockButton = new QPushButton(m_headerWidget);
    m_lockButton->setCursor(Qt::PointingHandCursor);
    m_lockButton->setFixedSize(26, 26);
    m_lockButton->setToolTip(m_model.locked ? "Déverrouiller" : "Verrouiller");

    m_syncButton = new QPushButton(m_headerWidget);
    m_syncButton->setCursor(Qt::ArrowCursor);
    m_syncButton->setFixedSize(26, 26);

    m_closeButton = new QPushButton(m_headerWidget);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFixedSize(26, 26);
    m_closeButton->setToolTip("Fermer / Supprimer");

    headerLayout->addWidget(m_menuButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
    headerLayout->addStretch();
    headerLayout->addWidget(m_pinButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_colorBadgeButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_plusButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_formatButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_lockButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_syncButton, 0, Qt::AlignVCenter);
    headerLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);

    // Text Editor
    m_textEdit = new QTextEdit(this);
    m_textEdit->setFrameStyle(QFrame::NoFrame);
    m_textEdit->setAcceptRichText(true);
    
    // Charger le HTML natif
    m_textEdit->setHtml(m_model.content);
    m_textEdit->setPlaceholderText("Écrivez quelque chose...");
    
    // Set padding via stylesheet or margins
    m_textEdit->document()->setDocumentMargin(12);
    m_textEdit->document()->setIndentWidth(16);

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
    connect(m_pinButton, &QPushButton::clicked, this, &StickyWindow::toggleAlwaysOnTop);
    connect(m_closeButton, &QPushButton::clicked, this, &StickyWindow::deleteNote);
    connect(m_lockButton, &QPushButton::clicked, this, &StickyWindow::toggleLocked);
    connect(m_plusButton, &QPushButton::clicked, this, &StickyWindow::newNoteRequested);
    connect(m_formatButton, &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.setAttribute(Qt::WA_TranslucentBackground);
        menu.setStyleSheet(
            "QMenu { background-color: palette(window); color: palette(text); border: 1px solid palette(mid); border-radius: 8px; padding: 4px; }"
            "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
            "QMenu::item:selected { background-color: palette(highlight); color: palette(highlightedText); }"
            "QMenu::separator { height: 1px; background: palette(mid); margin: 4px 0; }"
        );

        QAction* actBold = menu.addAction("Gras");
        actBold->setCheckable(true);
        actBold->setChecked(m_textEdit->fontWeight() == QFont::Bold);

        QAction* actItalic = menu.addAction("Italique");
        actItalic->setCheckable(true);
        actItalic->setChecked(m_textEdit->fontItalic());

        QAction* actUnderline = menu.addAction("Souligné");
        actUnderline->setCheckable(true);
        actUnderline->setChecked(m_textEdit->fontUnderline());

        QAction* actStrike = menu.addAction("Barré");
        actStrike->setCheckable(true);
        QTextCharFormat charFormat = m_textEdit->currentCharFormat();
        actStrike->setChecked(charFormat.fontStrikeOut());

        QAction* selected = menu.exec(m_formatButton->mapToGlobal(QPoint(0, m_formatButton->height())));
        if (selected == actBold) {
            QTextCharFormat format;
            format.setFontWeight(actBold->isChecked() ? QFont::Bold : QFont::Normal);
            m_textEdit->mergeCurrentCharFormat(format);
            saveTriggered();
        } else if (selected == actItalic) {
            QTextCharFormat format;
            format.setFontItalic(actItalic->isChecked());
            m_textEdit->mergeCurrentCharFormat(format);
            saveTriggered();
        } else if (selected == actUnderline) {
            QTextCharFormat format;
            format.setFontUnderline(actUnderline->isChecked());
            m_textEdit->mergeCurrentCharFormat(format);
            saveTriggered();
        } else if (selected == actStrike) {
            QTextCharFormat format;
            format.setFontStrikeOut(actStrike->isChecked());
            m_textEdit->mergeCurrentCharFormat(format);
            saveTriggered();
        }
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

        colorMenu.addAction("Ambre")->setData("#FEF08A");
        colorMenu.addAction("Azur")->setData("#BAE6FD");
        colorMenu.addAction("Sauge")->setData("#BBF7D0");
        colorMenu.addAction("Pêche")->setData("#FED7AA");
        colorMenu.addAction("Lavande")->setData("#E9D5FF");
        colorMenu.addAction("Rose")->setData("#FBCFE8");
        colorMenu.addAction("Ardoise")->setData("#E2E8F0");
        colorMenu.addAction("Blanc")->setData("#F8FAFC");
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

    m_model.color = normalizeNoteColor(m_model.color);
    QColor bgColor(m_model.color);
    QColor fgColor = getContrastColor(bgColor);

    // Colors & Stylesheets 2026 (Style .card-btn de la maquette avec petit carré visible)
    bool isDarkFg = (fgColor.lightness() < 128);
    QString btnBg = isDarkFg ? "rgba(0, 0, 0, 0.08)" : "rgba(255, 255, 255, 0.14)";
    QString btnHover = isDarkFg ? "rgba(0, 0, 0, 0.16)" : "rgba(255, 255, 255, 0.24)";
    QString btnPressed = isDarkFg ? "rgba(0, 0, 0, 0.24)" : "rgba(255, 255, 255, 0.34)";
    QString btnBorder = isDarkFg ? "1px solid rgba(0, 0, 0, 0.05)" : "1px solid rgba(255, 255, 255, 0.10)";

    QString style = QString(
        "StickyWindow { background: transparent; }"
        "QWidget#headerWidget { background: transparent; border-bottom: 1px solid rgba(0, 0, 0, 0.06); }"
        "QWidget#headerWidget QPushButton {"
        "    border: %3;"
        "    background-color: %4;"
        "    color: %1;"
        "    border-radius: 6px;"
        "    padding: 0px;"
        "    min-width: 26px;"
        "    max-width: 26px;"
        "    min-height: 26px;"
        "    max-height: 26px;"
        "}"
        "QWidget#headerWidget QPushButton:hover {"
        "    background-color: %5;"
        "}"
        "QWidget#headerWidget QPushButton:pressed {"
        "    background-color: %6;"
        "}"
        "QTextEdit { background: transparent; border: none; font-size: %2px; color: %1; padding: 8px 12px; }"
    ).arg(fgColor.name())
     .arg(m_currentFontSize)
     .arg(btnBorder)
     .arg(btnBg)
     .arg(btnHover)
     .arg(btnPressed);

    setStyleSheet(style);

    m_titleLabel->setStyleSheet(QString("font-weight: 700; font-size: 11px; letter-spacing: 0.5px; color: %1; opacity: 0.75;").arg(fgColor.name()));
    m_pinButton->setIcon(createPinIcon(m_model.alwaysOnTop, fgColor));
    m_pinButton->setIconSize(QSize(16, 16));
    m_closeButton->setIcon(createCloseIcon(fgColor));
    m_closeButton->setIconSize(QSize(16, 16));
    m_colorBadgeButton->setIcon(createColorBadgeIcon(m_model.color, fgColor));
    m_colorBadgeButton->setIconSize(QSize(16, 16));
    m_plusButton->setIcon(createPlusIcon(fgColor));
    m_plusButton->setIconSize(QSize(16, 16));
    m_formatButton->setIcon(createFormatIcon(fgColor));
    m_formatButton->setIconSize(QSize(16, 16));
    m_lockButton->setIcon(createLockIcon(m_model.locked, fgColor));
    m_lockButton->setIconSize(QSize(16, 16));

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
        m_syncButton->setIconSize(QSize(16, 16));
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

    QMenu* formatMenu = menu.addMenu("Format de liste");
    QAction* actionBulletList = formatMenu->addAction("Liste à puces");
    QAction* actionNumberedList = formatMenu->addAction("Liste numérotée");
    QAction* actionNormalText = formatMenu->addAction("Texte normal");
    formatMenu->addSeparator();
    QAction* actionConvertList = formatMenu->addAction("Convertir les tirets existants");
    menu.addSeparator();



    QMenu* colorMenu = menu.addMenu("Changer couleur");
    colorMenu->addAction("Ambre")->setData("#FEF08A");
    colorMenu->addAction("Azur")->setData("#BAE6FD");
    colorMenu->addAction("Sauge")->setData("#BBF7D0");
    colorMenu->addAction("Pêche")->setData("#FED7AA");
    colorMenu->addAction("Lavande")->setData("#E9D5FF");
    colorMenu->addAction("Rose")->setData("#FBCFE8");
    colorMenu->addAction("Ardoise")->setData("#E2E8F0");
    colorMenu->addAction("Blanc")->setData("#F8FAFC");
    colorMenu->addAction("Personnalisée...");

    QAction* actionPin = menu.addAction("Toujours devant");
    actionPin->setCheckable(true);
    actionPin->setChecked(m_model.alwaysOnTop);

    QAction* actionLock = menu.addAction("Verrouiller");
    actionLock->setCheckable(true);
    actionLock->setChecked(m_model.locked);

    bool autostartEnabled = AutostartManager::isEnabled();
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
    } else if (selected == actionBulletList) {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.beginEditBlock();
        QTextListFormat format;
        format.setStyle(QTextListFormat::ListDisc);
        format.setIndent(1);
        cursor.createList(format);
        cursor.endEditBlock();
        saveTriggered();
    } else if (selected == actionNumberedList) {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.beginEditBlock();
        QTextListFormat format;
        format.setStyle(QTextListFormat::ListDecimal);
        format.setIndent(1);
        cursor.createList(format);
        cursor.endEditBlock();
        saveTriggered();
    } else if (selected == actionNormalText) {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.beginEditBlock();
        QTextBlockFormat blockFormat;
        blockFormat.setIndent(0);
        cursor.setBlockFormat(blockFormat);
        if (QTextList* list = cursor.currentList()) {
            list->remove(cursor.block());
        }
        cursor.endEditBlock();
        saveTriggered();
    } else if (selected == actionConvertList) {
        convertPlainListToHtmlList();

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
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            
            // Raccourci Ctrl+B pour le gras
            if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_B) {
                QTextCharFormat format;
                format.setFontWeight(m_textEdit->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
                m_textEdit->mergeCurrentCharFormat(format);
                saveTriggered();
                return true;
            }

            // Raccourci Ctrl+I pour l'italique
            if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_I) {
                QTextCharFormat format;
                format.setFontItalic(!m_textEdit->fontItalic());
                m_textEdit->mergeCurrentCharFormat(format);
                saveTriggered();
                return true;
            }

            // Raccourci Ctrl+Shift+L pour basculer la liste à puces
            if (keyEvent->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && keyEvent->key() == Qt::Key_L) {
                QTextCursor cursor = m_textEdit->textCursor();
                cursor.beginEditBlock();
                if (cursor.currentList()) {
                    QTextList* list = cursor.currentList();
                    list->remove(cursor.block());
                } else {
                    QTextListFormat format;
                    format.setStyle(QTextListFormat::ListDisc);
                    format.setIndent(1);
                    cursor.insertList(format);
                }
                cursor.endEditBlock();
                return true;
            }

            // Détection de l'espace pour conversion automatique de "# ", "## ", "### ", "- ", "* ", ou "1. "
            if (keyEvent->key() == Qt::Key_Space) {
                QTextCursor cursor = m_textEdit->textCursor();
                QTextBlock block = cursor.block();
                int posInBlock = cursor.positionInBlock();
                QString text = block.text().left(posInBlock);
                
                if (text == "#") {
                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posInBlock);
                    cursor.removeSelectedText();
                    
                    QTextBlockFormat format;
                    format.setHeadingLevel(1);
                    cursor.setBlockFormat(format);
                    
                    QTextCharFormat charFormat;
                    charFormat.setFontPointSize(m_currentFontSize + 8);
                    charFormat.setFontWeight(QFont::Bold);
                    cursor.mergeBlockCharFormat(charFormat);
                    
                    cursor.endEditBlock();
                    saveTriggered();
                    return true;
                } else if (text == "##") {
                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posInBlock);
                    cursor.removeSelectedText();
                    
                    QTextBlockFormat format;
                    format.setHeadingLevel(2);
                    cursor.setBlockFormat(format);
                    
                    QTextCharFormat charFormat;
                    charFormat.setFontPointSize(m_currentFontSize + 4);
                    charFormat.setFontWeight(QFont::Bold);
                    cursor.mergeBlockCharFormat(charFormat);
                    
                    cursor.endEditBlock();
                    saveTriggered();
                    return true;
                } else if (text == "###") {
                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posInBlock);
                    cursor.removeSelectedText();
                    
                    QTextBlockFormat format;
                    format.setHeadingLevel(3);
                    cursor.setBlockFormat(format);
                    
                    QTextCharFormat charFormat;
                    charFormat.setFontPointSize(m_currentFontSize + 2);
                    charFormat.setFontWeight(QFont::Bold);
                    cursor.mergeBlockCharFormat(charFormat);
                    
                    cursor.endEditBlock();
                    saveTriggered();
                    return true;
                } else if (text == "-" || text == "*") {
                    QTextBlock prevBlock = block.previous();
                    QTextList* existingList = nullptr;
                    if (prevBlock.isValid() && prevBlock.textList() && prevBlock.textList()->format().style() == QTextListFormat::ListDisc) {
                        existingList = prevBlock.textList();
                    }

                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posInBlock);
                    cursor.removeSelectedText();
                    
                    QTextBlockFormat blockFormat = cursor.blockFormat();
                    blockFormat.setIndent(0);
                    cursor.setBlockFormat(blockFormat);
                    
                    if (existingList) {
                        existingList->add(cursor.block());
                    } else {
                        QTextListFormat format;
                        format.setStyle(QTextListFormat::ListDisc);
                        format.setIndent(1);
                        cursor.createList(format);
                    }
                    cursor.endEditBlock();
                    saveTriggered();
                    return true;
                } else if (text == "1.") {
                    QTextBlock prevBlock = block.previous();
                    QTextList* existingList = nullptr;
                    if (prevBlock.isValid() && prevBlock.textList() && prevBlock.textList()->format().style() == QTextListFormat::ListDecimal) {
                        existingList = prevBlock.textList();
                    }

                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posInBlock);
                    cursor.removeSelectedText();
                    
                    QTextBlockFormat blockFormat = cursor.blockFormat();
                    blockFormat.setIndent(0);
                    cursor.setBlockFormat(blockFormat);
                    
                    if (existingList) {
                        existingList->add(cursor.block());
                    } else {
                        QTextListFormat format;
                        format.setStyle(QTextListFormat::ListDecimal);
                        format.setIndent(1);
                        cursor.createList(format);
                    }
                    cursor.endEditBlock();
                    saveTriggered();
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void StickyWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Dessin précis de la note flottante avec coins arrondis et bordure douce
    QColor bgColor(normalizeNoteColor(m_model.color));
    p.setBrush(bgColor);
    p.setPen(QPen(QColor(0, 0, 0, 25), 1.2));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);

    // Dessiner une poignée de redimensionnement discrète dans le coin inférieur droit
    if (!m_model.locked) {
        QColor handleColor = getContrastColor(bgColor);
        handleColor.setAlpha(60);
        p.setPen(QPen(handleColor, 1.8, Qt::SolidLine, Qt::RoundCap));

        int w = width();
        int h = height();
        p.drawLine(w - 14, h - 5, w - 5, h - 14);
        p.drawLine(w - 9, h - 5, w - 5, h - 9);
    }
}

QIcon StickyWindow::createPenguinIcon() {
    return QIcon(":/logo.png");
}

QIcon StickyWindow::createPinIcon(bool pinned, const QColor& color) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    // Needle (Aiguille du bas)
    painter.drawLine(QPointF(12.0, 16.5), QPointF(12.0, 21.5));

    // Lucide Pushpin Path
    QPainterPath body;
    body.moveTo(5.5, 16.5);
    body.lineTo(18.5, 16.5);
    body.lineTo(18.5, 15.0);
    body.lineTo(15.0, 11.0);
    body.lineTo(15.0, 6.5);
    body.lineTo(16.5, 6.5);
    body.lineTo(16.5, 3.5);
    body.lineTo(7.5, 3.5);
    body.lineTo(7.5, 6.5);
    body.lineTo(9.0, 6.5);
    body.lineTo(9.0, 11.0);
    body.lineTo(5.5, 15.0);
    body.closeSubpath();

    if (pinned) {
        painter.fillPath(body, color);
    }
    painter.drawPath(body);

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createCloseIcon(const QColor& color) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(QPointF(7.0, 7.0), QPointF(17.0, 17.0));
    painter.drawLine(QPointF(17.0, 7.0), QPointF(7.0, 17.0));

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createLockIcon(bool locked, const QColor& color) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    // Corps du cadenas (Lucide Lock)
    QRectF bodyRect(5.5, 10.5, 13.0, 9.5);
    if (locked) {
        // Anse fermée
        QPainterPath shackle;
        shackle.moveTo(8.5, 10.5);
        shackle.lineTo(8.5, 6.5);
        shackle.arcTo(QRectF(8.5, 3.5, 7.0, 6.0), 180, -180);
        shackle.lineTo(15.5, 10.5);
        painter.drawPath(shackle);
    } else {
        // Anse ouverte levée
        QPainterPath shackle;
        shackle.moveTo(8.5, 10.5);
        shackle.lineTo(8.5, 5.5);
        shackle.arcTo(QRectF(8.5, 2.5, 7.0, 6.0), 180, -180);
        shackle.lineTo(15.5, 7.5);
        painter.drawPath(shackle);
    }

    painter.setBrush(locked ? QColor(color.red(), color.green(), color.blue(), 45) : Qt::NoBrush);
    painter.drawRoundedRect(bodyRect, 2.5, 2.5);

    // Trou de serrure
    painter.setBrush(color);
    painter.drawEllipse(QPointF(12.0, 14.0), 1.1, 1.1);
    painter.drawLine(QPointF(12.0, 14.5), QPointF(12.0, 17.0));

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createPlusIcon(const QColor& color) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(QPointF(6.0, 12.0), QPointF(18.0, 12.0));
    painter.drawLine(QPointF(12.0, 6.0), QPointF(12.0, 18.0));

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createColorBadgeIcon(const QString& hexColor, const QColor& strokeColor) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Anneau extérieur fin (Style sélecteur de couleur)
    painter.setPen(QPen(strokeColor, 1.6, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(4.0, 4.0, 16.0, 16.0));

    // 4 petits repères diagonaux discrets
    painter.setPen(QPen(strokeColor, 1.4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(6.5, 6.5), QPointF(8.5, 8.5));
    painter.drawLine(QPointF(17.5, 6.5), QPointF(15.5, 8.5));
    painter.drawLine(QPointF(17.5, 17.5), QPointF(15.5, 15.5));
    painter.drawLine(QPointF(6.5, 17.5), QPointF(8.5, 15.5));

    // Pastille centrale avec la couleur actuelle de la note
    QColor fill(hexColor);
    painter.setPen(QPen(strokeColor, 1.0));
    painter.setBrush(fill);
    painter.drawEllipse(QRectF(8.5, 8.5, 7.0, 7.0));

    painter.end();
    return QIcon(pixmap);
}

QIcon StickyWindow::createFormatIcon(const QColor& color) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    // Lucide Type (Typographie vectorielle indépendante des polices système)
    QPolygonF topBar;
    topBar << QPointF(5.5, 8.0) << QPointF(5.5, 5.0) << QPointF(18.5, 5.0) << QPointF(18.5, 8.0);
    painter.drawPolyline(topBar);

    painter.drawLine(QPointF(12.0, 5.0), QPointF(12.0, 19.0));
    painter.drawLine(QPointF(9.0, 19.0), QPointF(15.0, 19.0));

    painter.end();
    return QIcon(pixmap);
}



void StickyWindow::toggleAutostart() {
    AutostartManager::toggle();
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
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (!enabled) {
        painter.end();
        return QIcon();
    }

    QColor cloudColor = synced ? QColor("#10b981") : QColor("#f59e0b");

    // Nuage vectoriel Lucide
    QPen pen(cloudColor, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(QColor(cloudColor.red(), cloudColor.green(), cloudColor.blue(), 35));

    QPainterPath cloud;
    cloud.moveTo(6.5, 16.5);
    cloud.lineTo(17.5, 16.5);
    cloud.arcTo(QRectF(15.0, 12.0, 5.0, 5.0), 270, 180);
    cloud.arcTo(QRectF(13.0, 8.0, 6.0, 6.0), 0, 120);
    cloud.arcTo(QRectF(7.5, 6.5, 7.0, 7.0), 60, 150);
    cloud.arcTo(QRectF(4.0, 11.5, 5.5, 5.5), 150, 120);
    cloud.closeSubpath();
    painter.drawPath(cloud);

    if (synced) {
        painter.setPen(QPen(cloudColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(9.5, 12.0), QPointF(11.5, 14.0));
        painter.drawLine(QPointF(11.5, 14.0), QPointF(15.0, 9.5));
    } else {
        painter.setPen(QPen(cloudColor, 2.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(12.0, 10.0), QPointF(12.0, 12.5));
        painter.drawPoint(QPointF(12.0, 14.5));
    }

    painter.end();
    return QIcon(pixmap);
}

void StickyWindow::convertPlainListToHtmlList() {
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.beginEditBlock();
    
    QTextDocument* doc = m_textEdit->document();
    QTextBlock block = doc->end().previous();
    while (block.isValid()) {
        QString text = block.text().trimmed();
        if (text.startsWith("-") || text.startsWith("*")) {
            int prefixLen = 1;
            QString rawText = block.text();
            int firstNonSpace = 0;
            while (firstNonSpace < rawText.length() && rawText[firstNonSpace].isSpace()) {
                firstNonSpace++;
            }
            if (firstNonSpace < rawText.length() && (rawText[firstNonSpace] == '-' || rawText[firstNonSpace] == '*')) {
                int startPrefix = firstNonSpace;
                firstNonSpace++; // après le tiret/astérisque
                while (firstNonSpace < rawText.length() && rawText[firstNonSpace].isSpace()) {
                    firstNonSpace++;
                }
                prefixLen = firstNonSpace - startPrefix;
                
                QTextCursor blockCursor(block);
                blockCursor.movePosition(QTextCursor::StartOfBlock);
                blockCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startPrefix);
                blockCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefixLen);
                blockCursor.removeSelectedText();
                
                // Vérifier s'il y a une liste existante dans le bloc suivant (de type ListDisc)
                QTextBlock nextBlock = block.next();
                QTextList* existingList = nullptr;
                if (nextBlock.isValid() && nextBlock.textList() && nextBlock.textList()->format().style() == QTextListFormat::ListDisc) {
                    existingList = nextBlock.textList();
                }

                QTextBlockFormat blockFormat = blockCursor.blockFormat();
                blockFormat.setIndent(0);
                blockCursor.setBlockFormat(blockFormat);

                if (existingList) {
                    existingList->add(block);
                } else {
                    QTextListFormat format;
                    format.setStyle(QTextListFormat::ListDisc);
                    format.setIndent(1);
                    blockCursor.createList(format);
                }
            }
        } else if (text.startsWith("1.")) {
            QString rawText = block.text();
            int firstNonSpace = 0;
            while (firstNonSpace < rawText.length() && rawText[firstNonSpace].isSpace()) {
                firstNonSpace++;
            }
            if (firstNonSpace < rawText.length() - 1 && rawText[firstNonSpace] == '1' && rawText[firstNonSpace+1] == '.') {
                int startPrefix = firstNonSpace;
                firstNonSpace += 2; // après le "1."
                while (firstNonSpace < rawText.length() && rawText[firstNonSpace].isSpace()) {
                    firstNonSpace++;
                }
                int prefixLen = firstNonSpace - startPrefix;
                
                QTextCursor blockCursor(block);
                blockCursor.movePosition(QTextCursor::StartOfBlock);
                blockCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startPrefix);
                blockCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefixLen);
                blockCursor.removeSelectedText();
                
                // Vérifier s'il y a une liste existante dans le bloc suivant (de type ListDecimal)
                QTextBlock nextBlock = block.next();
                QTextList* existingList = nullptr;
                if (nextBlock.isValid() && nextBlock.textList() && nextBlock.textList()->format().style() == QTextListFormat::ListDecimal) {
                    existingList = nextBlock.textList();
                }

                QTextBlockFormat blockFormat = blockCursor.blockFormat();
                blockFormat.setIndent(0);
                blockCursor.setBlockFormat(blockFormat);

                if (existingList) {
                    existingList->add(block);
                } else {
                    QTextListFormat format;
                    format.setStyle(QTextListFormat::ListDecimal);
                    format.setIndent(1);
                    blockCursor.createList(format);
                }
            }
        }
        block = block.previous();
    }
    
    cursor.endEditBlock();
    saveTriggered();
}








