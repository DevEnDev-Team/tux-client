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

QString NoteCardWidget::getPlainPreview(const QString& htmlContent) const {
    QTextDocument doc;
    doc.setHtml(htmlContent);
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


// --- NotesListWindow ---
NotesListWindow::NotesListWindow(const std::vector<NoteModel>& notes, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_notes(notes)
{
    setWindowTitle("Liste de tous les Post-its");
    setWindowIcon(StickyWindow::createFoxIcon());
    resize(640, 480);
    setMinimumSize(420, 300);

    setupUi();
    updateNotesList(m_notes);
}

NotesListWindow::~NotesListWindow() {}

void NotesListWindow::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Style de la fenêtre (Thème moderne, fond épuré sombre-ish)
    setStyleSheet(
        "NotesListWindow { background-color: #1e1e24; }"
        "QLabel#mainTitle { font-weight: bold; font-size: 16px; color: #ffffff; }"
        "QLineEdit#searchEdit { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 8px; padding: 6px 12px; color: #ffffff; font-size: 12px; }"
        "QLineEdit#searchEdit:focus { border: 1px solid #90CAF9; }"
        "QListWidget { background-color: transparent; border: none; }"
        "QPushButton#closeBtn { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 6px; padding: 6px 16px; color: #ffffff; font-size: 12px; }"
        "QPushButton#closeBtn::hover { background-color: #3f3f50; }"
    );

    QLabel* label = new QLabel("Tableau de bord de vos Post-its", this);
    label->setObjectName("mainTitle");
    mainLayout->addWidget(label);

    // Barre de recherche
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Rechercher dans les Post-its...");
    mainLayout->addWidget(m_searchEdit);

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
            QMessageBox msgBox(QMessageBox::Question, "Supprimer", "Voulez-vous vraiment supprimer ce post-it ?", QMessageBox::Yes | QMessageBox::No, this);
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
                    doc.setHtml(note.content);
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
