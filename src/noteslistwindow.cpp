#include "noteslistwindow.h"
#include "stickywindow.h"
#include <QTextDocument>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QStyleOption>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStyleOption>

// --- NoteCardWidget ---
NoteCardWidget::NoteCardWidget(const NoteModel& note, QWidget* parent)
    : QWidget(parent)
    , m_id(note.id)
{
    setFixedHeight(60);

    // Layout principal horizontal pour former une ligne
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(12);

    // Layout vertical pour les textes (Titre + Aperçu)
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    textLayout->setContentsMargins(0, 0, 0, 0);

    // Calcul du contraste YIQ pour adapter la couleur des textes et boutons dynamiquement
    QColor bgColor(note.color);
    int yiq = ((bgColor.red() * 299) + (bgColor.green() * 587) + (bgColor.blue() * 114)) / 1000;
    QString textColorHex = (yiq >= 128) ? "#212121" : "#FAFAFA";
    QString textMutedColorHex = (yiq >= 128) ? "#555555" : "#CFD8DC";

    QString plainText = getPlainPreview(note.content);
    QString title = getTitleFromContent(plainText);
    QLabel* titleLabel = new QLabel(title, this);
    titleLabel->setStyleSheet(QString("font-weight: bold; font-size: 13px; color: %1; background: transparent; border: none;").arg(textColorHex));

    QString bodyPreview = plainText.mid(title.length()).trimmed();
    if (bodyPreview.isEmpty()) {
        bodyPreview = "...";
    }
    // Remplacer les retours à la ligne par des espaces pour l'aperçu sur une ligne
    bodyPreview = bodyPreview.replace('\n', ' ');
    if (bodyPreview.length() > 60) {
        bodyPreview = bodyPreview.left(60) + "...";
    }
    QLabel* previewLabel = new QLabel(bodyPreview, this);
    previewLabel->setStyleSheet(QString("font-size: 11px; color: %1; background: transparent; border: none;").arg(textMutedColorHex));

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(previewLabel);

    // Date de modification
    QLabel* dateLabel = new QLabel(note.lastModified.toString("dd/MM yyyy hh:mm"), this);
    dateLabel->setStyleSheet(QString("font-size: 10px; color: %1; background: transparent; border: none;").arg(textMutedColorHex));

    // Boutons d'actions rapides adaptés selon le contraste
    m_openButton = new QPushButton("Ouvrir", this);
    m_openButton->setCursor(Qt::PointingHandCursor);
    m_openButton->setFixedSize(55, 24);
    if (yiq >= 128) {
        m_openButton->setStyleSheet(
            "QPushButton { background-color: rgba(0, 0, 0, 0.08); border: none; border-radius: 4px; font-size: 10px; font-weight: bold; color: #212121; }"
            "QPushButton::hover { background-color: rgba(0, 0, 0, 0.16); }"
        );
    } else {
        m_openButton->setStyleSheet(
            "QPushButton { background-color: rgba(255, 255, 255, 0.12); border: none; border-radius: 4px; font-size: 10px; font-weight: bold; color: #FAFAFA; }"
            "QPushButton::hover { background-color: rgba(255, 255, 255, 0.24); }"
        );
    }

    m_deleteButton = new QPushButton("Supprimer", this);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    m_deleteButton->setFixedSize(70, 24);
    if (yiq >= 128) {
        m_deleteButton->setStyleSheet(
            "QPushButton { background-color: rgba(211, 47, 47, 0.1); border: none; border-radius: 4px; font-size: 10px; font-weight: bold; color: #d32f2f; }"
            "QPushButton::hover { background-color: rgba(211, 47, 47, 0.2); }"
        );
    } else {
        m_deleteButton->setStyleSheet(
            "QPushButton { background-color: rgba(255, 23, 68, 0.15); border: none; border-radius: 4px; font-size: 10px; font-weight: bold; color: #FF8A80; }"
            "QPushButton::hover { background-color: rgba(255, 23, 68, 0.3); }"
        );
    }

    // Détecter si la synchro est active globalement
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

    QLabel* syncIconLabel = nullptr;
    if (syncConfigured) {
        syncIconLabel = new QLabel(this);
        syncIconLabel->setPixmap(StickyWindow::createSyncIcon(note.synced, true).pixmap(16, 16));
        syncIconLabel->setToolTip(note.synced ? "Note synchronisée en ligne" : "Modifications locales non synchronisées");
        syncIconLabel->setStyleSheet("background: transparent; border: none;");
    }

    layout->addLayout(textLayout, 1);
    layout->addWidget(dateLabel);
    if (syncIconLabel) {
        layout->addWidget(syncIconLabel);
    }
    layout->addWidget(m_openButton);
    layout->addWidget(m_deleteButton);

    // Style de la ligne Post-it ciblant uniquement NoteCardWidget pour éviter les cascades indésirables
    QString borderStyle = QString(
        "NoteCardWidget { background-color: %1; border-radius: 8px; border: 1px solid rgba(0,0,0,0.06); }"
    ).arg(note.color);
    setStyleSheet(borderStyle);

    // Effet d'ombrage doux
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(6);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(0, 0, 0, 30));
    setGraphicsEffect(shadow);

    // Connexions des clics
    connect(m_openButton, &QPushButton::clicked, this, [this]() {
        emit openRequested(m_id);
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_id);
    });
}

QString NoteCardWidget::getPlainPreview(const QString& markdownContent) const {
    QTextDocument doc;
    doc.setMarkdown(markdownContent);
    return doc.toPlainText();
}

QString NoteCardWidget::getTitleFromContent(const QString& plainText) const {
    if (plainText.trimmed().isEmpty()) {
        return "Note vide";
    }
    QString firstLine = plainText.split('\n').first().trimmed();
    if (firstLine.length() > 25) {
        return firstLine.left(25) + "...";
    }
    return firstLine;
}

void NoteCardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

static QIcon createQrIcon() {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#ffffff"));

    // Corner 1: Top-Left
    painter.drawRect(4, 4, 8, 8);
    // Corner 2: Top-Right
    painter.drawRect(20, 4, 8, 8);
    // Corner 3: Bottom-Left
    painter.drawRect(4, 20, 8, 8);

    // Empty centers using CompositionMode_Clear
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.drawRect(6, 6, 4, 4);
    painter.drawRect(22, 6, 4, 4);
    painter.drawRect(6, 22, 4, 4);

    // Core of corners
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setBrush(QColor("#ffffff"));
    painter.drawRect(7, 7, 2, 2);
    painter.drawRect(23, 7, 2, 2);
    painter.drawRect(7, 23, 2, 2);

    // Random QR pixels
    painter.drawRect(14, 4, 2, 2);
    painter.drawRect(16, 8, 2, 4);
    painter.drawRect(12, 14, 4, 2);
    painter.drawRect(6, 14, 2, 2);
    painter.drawRect(14, 18, 4, 2);
    
    painter.drawRect(20, 14, 2, 2);
    painter.drawRect(24, 16, 2, 4);
    painter.drawRect(14, 24, 2, 4);
    painter.drawRect(24, 24, 4, 2);
    painter.drawRect(22, 22, 2, 2);

    return QIcon(pixmap);
}

static QIcon createThemeIcon(bool darkTheme) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (darkTheme) {
        // Sun icon (amber)
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#ffb300"));
        painter.drawEllipse(11, 11, 10, 10);
        
        painter.setPen(QPen(QColor("#ffb300"), 2));
        for (int i = 0; i < 8; ++i) {
            painter.save();
            painter.translate(16, 16);
            painter.rotate(i * 45);
            painter.drawLine(0, -9, 0, -13);
            painter.restore();
        }
    } else {
        // Moon icon (slate dark)
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#37474f"));
        
        QPainterPath path;
        path.addEllipse(8, 8, 16, 16);
        QPainterPath clip;
        clip.addEllipse(13, 6, 16, 16);
        QPainterPath crescent = path.subtracted(clip);
        painter.drawPath(crescent);
    }
    return QIcon(pixmap);
}


// --- NotesListWindow ---
NotesListWindow::NotesListWindow(const std::vector<NoteModel>& notes, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_notes(notes)
{
    m_darkTheme = loadThemeSetting();
    setWindowTitle("Liste de toutes les notes Tux-It");
    setWindowIcon(StickyWindow::createPenguinIcon());
    resize(640, 480);
    setMinimumSize(420, 300);

    setupUi();
    applyTheme(m_darkTheme);
    updateNotesList(m_notes);
}

NotesListWindow::~NotesListWindow() {}

void NotesListWindow::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    QLabel* label = new QLabel("Tableau de bord de vos notes Tux-It", this);
    label->setObjectName("mainTitle");
    mainLayout->addWidget(label);

    // Barre de recherche et boutons Nouveau / Synchro
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Rechercher dans les notes Tux-It...");
    
    QPushButton* mobileSyncBtn = new QPushButton(this);
    mobileSyncBtn->setObjectName("mobileSyncBtn");
    mobileSyncBtn->setIcon(createQrIcon());
    mobileSyncBtn->setIconSize(QSize(20, 20));
    mobileSyncBtn->setCursor(Qt::PointingHandCursor);
    mobileSyncBtn->setToolTip("Afficher le QR Code pour la synchronisation avec le téléphone");

    m_themeToggleBtn = new QPushButton(this);
    m_themeToggleBtn->setObjectName("themeToggleBtn");
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setToolTip("Changer de thème (Clair / Sombre)");

    QPushButton* newNoteBtn = new QPushButton("Nouveau Tux-It", this);
    newNoteBtn->setObjectName("newNoteBtn");
    newNoteBtn->setCursor(Qt::PointingHandCursor);
    
    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(mobileSyncBtn);
    searchLayout->addWidget(m_themeToggleBtn);
    searchLayout->addWidget(newNoteBtn);
    mainLayout->addLayout(searchLayout);

    // Grille de Post-its (QListWidget en mode liste verticale)
    m_listWidget = new QListWidget(this);
    m_listWidget->setSpacing(8);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    
    // Désactiver le contour de focus orange sur certains thèmes
    m_listWidget->setFocusPolicy(Qt::NoFocus);

    mainLayout->addWidget(m_listWidget, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_closeButton = new QPushButton("Fermer", this);
    m_closeButton->setObjectName("closeBtn");
    m_closeButton->setCursor(Qt::PointingHandCursor);

    btnLayout->addStretch();
    btnLayout->addWidget(m_closeButton);
    mainLayout->addLayout(btnLayout);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &NotesListWindow::filterNotes);
    connect(mobileSyncBtn, &QPushButton::clicked, this, &NotesListWindow::mobileSyncRequested);
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &NotesListWindow::toggleTheme);
    connect(newNoteBtn, &QPushButton::clicked, this, &NotesListWindow::newNoteRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &NotesListWindow::close);
}

void NotesListWindow::updateNotesList(const std::vector<NoteModel>& notes) {
    m_notes = notes;
    m_listWidget->clear();

    for (const auto& note : m_notes) {
        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(0, 60)); // Hauteur fixe de 60px pour chaque ligne

        NoteCardWidget* card = new NoteCardWidget(note, this);

        // Relayer les requêtes de la carte vers la fenêtre principale
        connect(card, &NoteCardWidget::openRequested, this, &NotesListWindow::openNoteRequested);
        connect(card, &NoteCardWidget::deleteRequested, this, [this](const QString& id) {
            QMessageBox msgBox(QMessageBox::Question, "Supprimer", "Voulez-vous vraiment supprimer cette note Tux-It ?", QMessageBox::Yes | QMessageBox::No, this);
            msgBox.setWindowModality(Qt::WindowModal);
            if (msgBox.exec() == QMessageBox::Yes) {
                emit deleteNoteRequested(id);
            }
        });

        m_listWidget->setItemWidget(item, card);
    }
    
    // Relancer le filtre actuel si du texte est présent
    filterNotes(m_searchEdit->text());
}

void NotesListWindow::filterNotes(const QString& text) {
    QString query = text.trimmed().toLower();

    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem* item = m_listWidget->item(i);
        NoteCardWidget* card = qobject_cast<NoteCardWidget*>(m_listWidget->itemWidget(item));
        if (card) {
            // Retrouver le modèle associé dans m_notes
            bool matches = false;
            for (const auto& note : m_notes) {
                if (note.id == card->id()) {
                    QTextDocument doc;
                    doc.setMarkdown(note.content);
                    QString plainText = doc.toPlainText().toLower();
                    
                    if (plainText.contains(query) || note.tags.join(" ").toLower().contains(query)) {
                        matches = true;
                    }
                    break;
                }
            }
            item->setHidden(!matches);
        }
    }
}

void NotesListWindow::applyTheme(bool dark) {
    m_darkTheme = dark;
    m_themeToggleBtn->setIcon(createThemeIcon(dark));
    m_themeToggleBtn->setIconSize(QSize(20, 20));

    if (dark) {
        setStyleSheet(
            "NotesListWindow { background-color: #1e1e24; }"
            "QLabel#mainTitle { font-weight: bold; font-size: 16px; color: #ffffff; }"
            "QLineEdit#searchEdit { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 8px; padding: 0 12px; height: 32px; color: #ffffff; font-size: 12px; }"
            "QLineEdit#searchEdit:focus { border: 1px solid #7c4dff; }"
            "QPushButton#newNoteBtn { background-color: #10b981; border: none; border-radius: 8px; padding: 0 16px; height: 32px; color: #ffffff; font-weight: bold; font-size: 12px; }"
            "QPushButton#newNoteBtn::hover { background-color: #059669; }"
            "QPushButton#mobileSyncBtn { background-color: #7c4dff; border: none; border-radius: 8px; padding: 0; width: 32px; height: 32px; color: #ffffff; }"
            "QPushButton#mobileSyncBtn::hover { background-color: #651fff; }"
            "QPushButton#themeToggleBtn { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 8px; padding: 0; width: 32px; height: 32px; color: #ffffff; }"
            "QPushButton#themeToggleBtn::hover { background-color: #3f3f50; }"
            "QListWidget { background-color: transparent; border: none; }"
            "QPushButton#closeBtn { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 6px; padding: 6px 16px; color: #ffffff; font-size: 12px; }"
            "QPushButton#closeBtn::hover { background-color: #3f3f50; }"
        );
    } else {
        setStyleSheet(
            "NotesListWindow { background-color: #f5f5f7; }"
            "QLabel#mainTitle { font-weight: bold; font-size: 16px; color: #1d1d1f; }"
            "QLineEdit#searchEdit { background-color: #ffffff; border: 1px solid #d2d2d7; border-radius: 8px; padding: 0 12px; height: 32px; color: #1d1d1f; font-size: 12px; }"
            "QLineEdit#searchEdit:focus { border: 1px solid #7c4dff; }"
            "QPushButton#newNoteBtn { background-color: #10b981; border: none; border-radius: 8px; padding: 0 16px; height: 32px; color: #ffffff; font-weight: bold; font-size: 12px; }"
            "QPushButton#newNoteBtn::hover { background-color: #059669; }"
            "QPushButton#mobileSyncBtn { background-color: #7c4dff; border: none; border-radius: 8px; padding: 0; width: 32px; height: 32px; color: #ffffff; }"
            "QPushButton#mobileSyncBtn::hover { background-color: #651fff; }"
            "QPushButton#themeToggleBtn { background-color: #ffffff; border: 1px solid #d2d2d7; border-radius: 8px; padding: 0; width: 32px; height: 32px; color: #1d1d1f; }"
            "QPushButton#themeToggleBtn::hover { background-color: #e8e8ed; }"
            "QListWidget { background-color: transparent; border: none; }"
            "QPushButton#closeBtn { background-color: #ffffff; border: 1px solid #d2d2d7; border-radius: 6px; padding: 6px 16px; color: #1d1d1f; font-size: 12px; }"
            "QPushButton#closeBtn::hover { background-color: #e8e8ed; }"
        );
    }
}

void NotesListWindow::toggleTheme() {
    bool nextTheme = !m_darkTheme;
    applyTheme(nextTheme);
    saveThemeSetting(nextTheme);
}

bool NotesListWindow::loadThemeSetting() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        file.close();
        return obj.value("theme").toString("dark") == "dark";
    }
    return true;
}

void NotesListWindow::saveThemeSetting(bool dark) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";
    QFile file(configPath);
    QJsonObject obj;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        obj = doc.object();
        file.close();
    }
    obj["theme"] = dark ? "dark" : "light";
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}
