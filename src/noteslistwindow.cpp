#include "noteslistwindow.h"
#include "stickywindow.h"
#include "updatemanager.h"
#include "autostartmanager.h"
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
#include <QResizeEvent>
#include <QScrollBar>

// --- Helper Functions for Icons ---
static QIcon createQrIcon(bool darkTheme) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    
    QColor qrColor = darkTheme ? QColor("#ffffff") : QColor("#0f172a");
    painter.setPen(Qt::NoPen);
    painter.setBrush(qrColor);

    // Corner 1: Top-Left
    painter.drawRect(4, 4, 8, 8);
    // Corner 2: Top-Right
    painter.drawRect(20, 4, 8, 8);
    // Corner 3: Bottom-Left
    painter.drawRect(4, 20, 8, 8);

    // Empty centers
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.drawRect(6, 6, 4, 4);
    painter.drawRect(22, 6, 4, 4);
    painter.drawRect(6, 22, 4, 4);

    // Core of corners
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setBrush(qrColor);
    painter.drawRect(7, 7, 2, 2);
    painter.drawRect(23, 7, 2, 2);
    painter.drawRect(7, 23, 2, 2);

    // Pixels
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
        painter.setBrush(QColor("#fbbf24"));
        painter.drawEllipse(11, 11, 10, 10);
        
        painter.setPen(QPen(QColor("#fbbf24"), 2, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 8; ++i) {
            painter.save();
            painter.translate(16, 16);
            painter.rotate(i * 45);
            painter.drawLine(0, -9, 0, -13);
            painter.restore();
        }
    } else {
        // Moon icon (slate)
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#334155"));
        
        QPainterPath path;
        path.addEllipse(8, 8, 16, 16);
        QPainterPath clip;
        clip.addEllipse(13, 6, 16, 16);
        QPainterPath crescent = path.subtracted(clip);
        painter.drawPath(crescent);
    }
    return QIcon(pixmap);
}

static QIcon createViewModeIcon(bool isGridView, bool darkTheme) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QColor color = darkTheme ? QColor("#e2e8f0") : QColor("#334155");

    if (isGridView) {
        // Mode Grille -> Dessiner 4 carrés
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(6, 6, 8, 8, 2, 2);
        painter.drawRoundedRect(18, 6, 8, 8, 2, 2);
        painter.drawRoundedRect(6, 18, 8, 8, 2, 2);
        painter.drawRoundedRect(18, 18, 8, 8, 2, 2);
    } else {
        // Mode Liste -> Dessiner 3 barres horizontales
        painter.setPen(QPen(color, 2.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(6, 8, 26, 8);
        painter.drawLine(6, 16, 26, 16);
        painter.drawLine(6, 24, 26, 24);
    }
    return QIcon(pixmap);
}

static void getThemeColorsForNote(const QString& rawColor, bool isDark, QColor& bg, QColor& text, QColor& muted, QColor& border) {
    QString hex = normalizeNoteColor(rawColor).toUpper();
    if (isDark) {
        if (hex == "#FEF08A") { bg = QColor("#2e2614"); text = QColor("#fef08a"); muted = QColor("#fde047"); border = QColor(254, 240, 138, 55); }
        else if (hex == "#BBF7D0") { bg = QColor("#132b1d"); text = QColor("#bbf7d0"); muted = QColor("#86efac"); border = QColor(187, 247, 208, 55); }
        else if (hex == "#BAE6FD") { bg = QColor("#11293a"); text = QColor("#bae6fd"); muted = QColor("#7dd3fc"); border = QColor(186, 230, 253, 55); }
        else if (hex == "#FED7AA") { bg = QColor("#311e13"); text = QColor("#fed7aa"); muted = QColor("#fdba74"); border = QColor(254, 215, 170, 55); }
        else if (hex == "#E9D5FF") { bg = QColor("#27173a"); text = QColor("#e9d5ff"); muted = QColor("#d8b4fe"); border = QColor(233, 213, 255, 55); }
        else if (hex == "#FBCFE8") { bg = QColor("#351424"); text = QColor("#fbcfe8"); muted = QColor("#f472b6"); border = QColor(251, 207, 232, 55); }
        else { bg = QColor("#1e2530"); text = QColor("#f1f5f9"); muted = QColor("#94a3b8"); border = QColor(241, 245, 249, 45); }
    } else {
        if (hex == "#FEF08A") { bg = QColor("#fef08a"); text = QColor("#713f12"); muted = QColor("#a16207"); border = QColor(161, 98, 7, 55); }
        else if (hex == "#BBF7D0") { bg = QColor("#bbf7d0"); text = QColor("#14532d"); muted = QColor("#166534"); border = QColor(22, 101, 52, 55); }
        else if (hex == "#BAE6FD") { bg = QColor("#bae6fd"); text = QColor("#0c4a6e"); muted = QColor("#0369a1"); border = QColor(3, 105, 161, 55); }
        else if (hex == "#FED7AA") { bg = QColor("#fed7aa"); text = QColor("#7c2d12"); muted = QColor("#c2410c"); border = QColor(194, 65, 12, 55); }
        else if (hex == "#E9D5FF") { bg = QColor("#e9d5ff"); text = QColor("#3b1a54"); muted = QColor("#581c87"); border = QColor(90, 30, 130, 55); }
        else if (hex == "#FBCFE8") { bg = QColor("#fbcfe8"); text = QColor("#831843"); muted = QColor("#9d174d"); border = QColor(157, 23, 77, 55); }
        else { bg = QColor("#e2e8f0"); text = QColor("#0f172a"); muted = QColor("#475569"); border = QColor(51, 65, 85, 55); }
    }
}

// --- NoteCardWidget Implementation ---
NoteCardWidget::NoteCardWidget(const NoteModel& note, bool isGridView, bool darkTheme, QWidget* parent)
    : QWidget(parent)
    , m_note(note)
    , m_id(note.id)
    , m_color(normalizeNoteColor(note.color))
    , m_isGridView(isGridView)
{
    setCursor(Qt::PointingHandCursor);

    QColor bgColor, textColor, mutedColor, borderColor;
    getThemeColorsForNote(m_color, darkTheme, bgColor, textColor, mutedColor, borderColor);
    QString textColorHex = textColor.name();
    QString textMutedHex = mutedColor.name();

    QString plainText = getPlainPreview(note.content);
    QString title = getTitleFromContent(plainText);

    // Boutons Ouvrir & Supprimer
    m_openButton = new QPushButton("Ouvrir", this);
    m_openButton->setCursor(Qt::PointingHandCursor);
    m_openButton->setFixedSize(56, 26);

    m_deleteButton = new QPushButton("Supprimer", this);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    m_deleteButton->setFixedSize(70, 26);

    if (darkTheme) {
        m_openButton->setStyleSheet(
            "QPushButton { background-color: rgba(255, 255, 255, 0.12); border: none; border-radius: 6px; font-size: 11px; font-weight: 600; color: #f9fafb; }"
            "QPushButton::hover { background-color: rgba(255, 255, 255, 0.22); }"
        );
        m_deleteButton->setStyleSheet(
            "QPushButton { background-color: rgba(248, 113, 113, 0.18); border: none; border-radius: 6px; font-size: 11px; font-weight: 600; color: #fca5a5; }"
            "QPushButton::hover { background-color: rgba(248, 113, 113, 0.32); }"
        );
    } else {
        m_openButton->setStyleSheet(
            "QPushButton { background-color: rgba(0, 0, 0, 0.07); border: none; border-radius: 6px; font-size: 11px; font-weight: 600; color: #1f2937; }"
            "QPushButton::hover { background-color: rgba(0, 0, 0, 0.14); }"
        );
        m_deleteButton->setStyleSheet(
            "QPushButton { background-color: rgba(220, 38, 38, 0.1); border: none; border-radius: 6px; font-size: 11px; font-weight: 600; color: #b91c1c; }"
            "QPushButton::hover { background-color: rgba(220, 38, 38, 0.2); }"
        );
    }

    if (m_isGridView) {
        // --- MODE GRILLE BENTO (Carte 2026) ---
        setFixedHeight(150);
        setFixedWidth(240);

        QVBoxLayout* cardLayout = new QVBoxLayout(this);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(8);

        // Header de la carte : Titre à gauche, Actions à droite
        QHBoxLayout* headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(6);

        QLabel* titleLabel = new QLabel(title, this);
        titleLabel->setStyleSheet(QString("font-weight: 700; font-size: 13.5px; color: %1; background: transparent; border: none;").arg(textColorHex));
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        headerLayout->addWidget(titleLabel, 1);
        headerLayout->addWidget(m_openButton);
        headerLayout->addWidget(m_deleteButton);

        // Corps du texte (Preview jusqu'à 3 lignes)
        QString bodyPreview = plainText.mid(title.length()).trimmed();
        if (bodyPreview.isEmpty()) {
            bodyPreview = "Note vide...";
        }
        bodyPreview.replace('\n', ' ');
        if (bodyPreview.length() > 95) {
            bodyPreview = bodyPreview.left(95) + "...";
        }

        QLabel* previewLabel = new QLabel(bodyPreview, this);
        previewLabel->setWordWrap(true);
        previewLabel->setStyleSheet(QString("font-size: 12px; line-height: 1.4; color: %1; background: transparent; border: none;").arg(textMutedHex));
        previewLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        // Footer : Date & Tags
        QHBoxLayout* footerLayout = new QHBoxLayout();
        footerLayout->setSpacing(6);

        QLabel* dateLabel = new QLabel(getFormattedDate(note.lastModified), this);
        dateLabel->setStyleSheet(QString("font-size: 10.5px; color: %1; background: transparent; border: none;").arg(textMutedHex));
        dateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        footerLayout->addWidget(dateLabel);

        if (!note.tags.isEmpty()) {
            QLabel* tagLabel = new QLabel(note.tags.join(" "), this);
            tagLabel->setStyleSheet(QString("font-size: 10px; font-weight: 600; color: %1; background: rgba(0,0,0,0.08); padding: 2px 6px; border-radius: 4px; border: none;").arg(textColorHex));
            tagLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            footerLayout->addStretch();
            footerLayout->addWidget(tagLabel);
        } else {
            footerLayout->addStretch();
        }

        cardLayout->addLayout(headerLayout);
        cardLayout->addWidget(previewLabel, 1);
        cardLayout->addLayout(footerLayout);

        setStyleSheet(QString("NoteCardWidget { background-color: %1; border-radius: 14px; border: 1px solid %2; }").arg(bgColor.name(), borderColor.name(QColor::HexArgb)));
    } else {
        // --- MODE LISTE (Fidèle à la maquette) ---
        setFixedHeight(48);

        QHBoxLayout* listLayout = new QHBoxLayout(this);
        listLayout->setContentsMargins(16, 6, 16, 6);
        listLayout->setSpacing(16);

        // 1. Titre (colonne gauche 200px)
        QLabel* titleLabel = new QLabel(title, this);
        titleLabel->setFixedWidth(200);
        titleLabel->setStyleSheet(QString("font-weight: 700; font-size: 13.5px; color: %1; background: transparent; border: none;").arg(textColorHex));
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        // 2. Extrait du texte (au milieu, fluide sur 1 ligne avec points de suspension)
        QString bodyPreview = plainText.mid(title.length()).trimmed().replace('\n', ' ');
        if (bodyPreview.length() > 80) {
            bodyPreview = bodyPreview.left(80) + "...";
        }
        if (bodyPreview.isEmpty()) bodyPreview = "...";

        QLabel* previewLabel = new QLabel(bodyPreview, this);
        previewLabel->setStyleSheet(QString("font-size: 12px; color: %1; background: transparent; border: none;").arg(textMutedHex));
        previewLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        // 3. Métadonnées (Date + Tags)
        QHBoxLayout* metaLayout = new QHBoxLayout();
        metaLayout->setSpacing(8);

        QLabel* dateLabel = new QLabel(getFormattedDate(note.lastModified), this);
        dateLabel->setStyleSheet(QString("font-size: 11px; font-family: monospace; color: %1; background: transparent; border: none;").arg(textMutedHex));
        dateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        metaLayout->addWidget(dateLabel);

        if (!note.tags.isEmpty()) {
            QLabel* tagLabel = new QLabel(note.tags.join(" "), this);
            tagLabel->setStyleSheet(QString("font-size: 10px; font-weight: 600; color: %1; background: rgba(0,0,0,0.08); padding: 2px 6px; border-radius: 4px; border: none;").arg(textColorHex));
            tagLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            metaLayout->addWidget(tagLabel);
        }

        listLayout->addWidget(titleLabel);
        listLayout->addWidget(previewLabel, 1);
        listLayout->addLayout(metaLayout);
        listLayout->addWidget(m_openButton);
        listLayout->addWidget(m_deleteButton);

        setStyleSheet(QString("NoteCardWidget { background-color: %1; border-radius: 10px; border: 1px solid %2; }").arg(bgColor.name(), borderColor.name(QColor::HexArgb)));
    }

    // Ombre portée subtile
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 25));
    setGraphicsEffect(shadow);

    // Connexions
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
    if (firstLine.length() > 28) {
        return firstLine.left(28) + "...";
    }
    return firstLine;
}

QString NoteCardWidget::getFormattedDate(const QDateTime& dt) const {
    QDate today = QDate::currentDate();
    if (dt.date() == today) {
        return "Aujourd'hui " + dt.time().toString("hh:mm");
    } else if (dt.date() == today.addDays(-1)) {
        return "Hier " + dt.time().toString("hh:mm");
    }
    return dt.toString("dd/MM/yyyy hh:mm");
}

void NoteCardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void NoteCardWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (child != m_deleteButton && (!m_deleteButton || !m_deleteButton->isAncestorOf(child))) {
            emit openRequested(m_id);
            event->accept();
            return;
        }
    }
    QWidget::mouseReleaseEvent(event);
}


// --- NotesListWindow Implementation ---
NotesListWindow::NotesListWindow(const std::vector<NoteModel>& notes, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_notes(notes)
{
    m_darkTheme = loadThemeSetting();
    m_isGridView = loadViewModeSetting();

    setWindowTitle("Tux-It Workspace");
    setWindowIcon(StickyWindow::createPenguinIcon());
    resize(820, 580);
    setMinimumSize(480, 360);

    setupUi();
    applyTheme(m_darkTheme);
    populateList();
}

NotesListWindow::~NotesListWindow() {}

void NotesListWindow::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    // 1. HEADER (Logo + Titre + Sync status + Boutons contrôles)
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QLabel* logoLabel = new QLabel(this);
    logoLabel->setPixmap(StickyWindow::createPenguinIcon().pixmap(28, 28));
    logoLabel->setFixedSize(28, 28);

    QVBoxLayout* titlesLayout = new QVBoxLayout();
    titlesLayout->setSpacing(1);
    titlesLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel("Tux-It Workspace", this);
    m_titleLabel->setObjectName("mainTitle");

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setObjectName("subTitle");

    titlesLayout->addWidget(m_titleLabel);
    titlesLayout->addWidget(m_subtitleLabel);

    headerLayout->addWidget(logoLabel);
    headerLayout->addLayout(titlesLayout);
    headerLayout->addStretch();

    // Badge Synchro
    m_syncBadge = new QLabel(this);
    m_syncBadge->setObjectName("syncBadge");
    headerLayout->addWidget(m_syncBadge);

    // Bouton Démarrage automatique (Autostart)
    m_autostartBtn = new QPushButton(this);
    m_autostartBtn->setObjectName("autostartBtn");
    m_autostartBtn->setIconSize(QSize(18, 18));
    m_autostartBtn->setCursor(Qt::PointingHandCursor);
    connect(m_autostartBtn, &QPushButton::clicked, this, &NotesListWindow::toggleAutostart);

    // Bouton Synchro Mobile QR
    m_mobileSyncBtn = new QPushButton(this);
    m_mobileSyncBtn->setObjectName("mobileSyncBtn");
    m_mobileSyncBtn->setIcon(createQrIcon(m_darkTheme));
    m_mobileSyncBtn->setIconSize(QSize(18, 18));
    m_mobileSyncBtn->setCursor(Qt::PointingHandCursor);
    m_mobileSyncBtn->setToolTip("Afficher le QR Code pour la synchronisation avec le téléphone");

    // Bouton Bascule Thème Clair / Sombre
    m_themeToggleBtn = new QPushButton(this);
    m_themeToggleBtn->setObjectName("themeToggleBtn");
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setToolTip("Changer de thème (Clair / Sombre)");

    // Bouton Mise à jour
    m_updateBtn = new QPushButton(this);
    m_updateBtn->setObjectName("updateBtn");
    m_updateBtn->setCursor(Qt::PointingHandCursor);
    m_updateBtn->setIcon(UpdateManager::createUpdateIcon(false, m_darkTheme));
    m_updateBtn->setIconSize(QSize(18, 18));
    m_updateBtn->setToolTip("Vérifier les mises à jour");

    headerLayout->addWidget(m_autostartBtn);
    headerLayout->addWidget(m_mobileSyncBtn);
    headerLayout->addWidget(m_themeToggleBtn);
    headerLayout->addWidget(m_updateBtn);

    mainLayout->addLayout(headerLayout);

    // 2. TOOLBAR (Recherche + Toggle Vue + Nouveau Note)
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(10);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText("Rechercher dans les notes ou les tags... (Ctrl+F)");

    m_viewToggleBtn = new QPushButton(this);
    m_viewToggleBtn->setObjectName("viewToggleBtn");
    m_viewToggleBtn->setCursor(Qt::PointingHandCursor);
    m_viewToggleBtn->setIcon(createViewModeIcon(m_isGridView, m_darkTheme));
    m_viewToggleBtn->setIconSize(QSize(18, 18));
    m_viewToggleBtn->setToolTip(m_isGridView ? "Passer en vue Liste compacte" : "Passer en vue Grille");

    m_newNoteBtn = new QPushButton("+ Nouveau Tux-It", this);
    m_newNoteBtn->setObjectName("newNoteBtn");
    m_newNoteBtn->setCursor(Qt::PointingHandCursor);

    toolbarLayout->addWidget(m_searchEdit, 1);
    toolbarLayout->addWidget(m_viewToggleBtn);
    toolbarLayout->addWidget(m_newNoteBtn);

    mainLayout->addLayout(toolbarLayout);

    // 3. FILTRES PAR COULEUR (Chips 2026)
    m_filtersLayout = new QHBoxLayout();
    m_filtersLayout->setSpacing(6);

    struct ColorFilterDef {
        QString name;
        QString hex;
    };
    QList<ColorFilterDef> filterDefs = {
        {"Toutes", ""},
        {"Ambre", "#FEF08A"},
        {"Azur", "#BAE6FD"},
        {"Sauge", "#BBF7D0"},
        {"Pêche", "#FED7AA"},
        {"Lavande", "#E9D5FF"},
        {"Rose", "#FBCFE8"},
        {"Ardoise", "#E2E8F0"}
    };

    for (const auto& def : filterDefs) {
        QPushButton* chip = new QPushButton(def.name, this);
        chip->setObjectName("filterChip");
        chip->setCursor(Qt::PointingHandCursor);
        chip->setProperty("colorHex", def.hex);

        connect(chip, &QPushButton::clicked, this, [this, chip]() {
            QString colorHex = chip->property("colorHex").toString();
            onColorFilterClicked(colorHex);
        });

        m_filterChips.append(chip);
        m_filtersLayout->addWidget(chip);
    }
    m_filtersLayout->addStretch();
    mainLayout->addLayout(m_filtersLayout);

    // 4. LISTE OU GRILLE DE NOTES (QListWidget moderne)
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mainLayout->addWidget(m_listWidget, 1);

    // 5. FOOTER (Fermer)
    QHBoxLayout* footerLayout = new QHBoxLayout();
    m_closeButton = new QPushButton("Fermer", this);
    m_closeButton->setObjectName("closeBtn");
    m_closeButton->setCursor(Qt::PointingHandCursor);

    footerLayout->addStretch();
    footerLayout->addWidget(m_closeButton);
    mainLayout->addLayout(footerLayout);

    // Connexions
    connect(m_searchEdit, &QLineEdit::textChanged, this, &NotesListWindow::filterNotes);
    connect(m_viewToggleBtn, &QPushButton::clicked, this, &NotesListWindow::toggleViewMode);
    connect(m_mobileSyncBtn, &QPushButton::clicked, this, &NotesListWindow::mobileSyncRequested);
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &NotesListWindow::toggleTheme);
    connect(m_updateBtn, &QPushButton::clicked, this, &NotesListWindow::onUpdateBtnClicked);
    connect(m_newNoteBtn, &QPushButton::clicked, this, &NotesListWindow::newNoteRequested);
    connect(m_closeButton, &QPushButton::clicked, this, &NotesListWindow::close);

    updateSyncBadge();
}

void NotesListWindow::populateList() {
    m_listWidget->clear();

    if (m_isGridView) {
        m_listWidget->setViewMode(QListView::IconMode);
        m_listWidget->setResizeMode(QListView::Adjust);
        m_listWidget->setWordWrap(true);
        m_listWidget->setMovement(QListView::Static);
        m_listWidget->setSpacing(12);
    } else {
        m_listWidget->setViewMode(QListView::ListMode);
        m_listWidget->setResizeMode(QListView::Adjust);
        m_listWidget->setSpacing(8);
    }

    for (const auto& note : m_notes) {
        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        if (m_isGridView) {
            item->setSizeHint(QSize(240, 150));
        } else {
            item->setSizeHint(QSize(m_listWidget->viewport()->width() - 16, 48));
        }

        NoteCardWidget* card = new NoteCardWidget(note, m_isGridView, m_darkTheme, this);

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

    // Mise à jour du sous-titre
    int count = static_cast<int>(m_notes.size());
    QString countStr = QString("%1 note%2 sauvegardée%2 · Prêt")
                           .arg(count)
                           .arg(count > 1 ? "s" : "");
    m_subtitleLabel->setText(countStr);

    filterNotes();
}

void NotesListWindow::updateNotesList(const std::vector<NoteModel>& notes) {
    m_notes = notes;
    populateList();
}

void NotesListWindow::filterNotes() {
    QString query = m_searchEdit->text().trimmed().toLower();

    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem* item = m_listWidget->item(i);
        NoteCardWidget* card = qobject_cast<NoteCardWidget*>(m_listWidget->itemWidget(item));
        if (card) {
            bool matchesText = query.isEmpty();
            bool matchesColor = m_selectedColorFilter.isEmpty();

            const NoteModel& note = card->noteModel();
            if (!query.isEmpty()) {
                QTextDocument doc;
                doc.setHtml(note.content);
                QString plain = doc.toPlainText().toLower();
                if (plain.contains(query) || note.tags.join(" ").toLower().contains(query)) {
                    matchesText = true;
                }
            }

            if (!m_selectedColorFilter.isEmpty()) {
                QString noteColor = normalizeNoteColor(note.color).toUpper();
                if (noteColor == m_selectedColorFilter.toUpper()) {
                    matchesColor = true;
                }
            }

            item->setHidden(!(matchesText && matchesColor));
        }
    }
}

void NotesListWindow::onColorFilterClicked(const QString& colorHex) {
    m_selectedColorFilter = colorHex;

    for (QPushButton* chip : m_filterChips) {
        bool isActive = (chip->property("colorHex").toString() == colorHex);
        chip->setProperty("active", isActive);
        chip->style()->unpolish(chip);
        chip->style()->polish(chip);
    }

    filterNotes();
}

void NotesListWindow::toggleViewMode() {
    m_isGridView = !m_isGridView;
    saveViewModeSetting(m_isGridView);
    m_viewToggleBtn->setIcon(createViewModeIcon(m_isGridView, m_darkTheme));
    m_viewToggleBtn->setToolTip(m_isGridView ? "Passer en vue Liste compacte" : "Passer en vue Grille");
    populateList();
}

void NotesListWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (!m_isGridView && m_listWidget) {
        int w = m_listWidget->viewport()->width() - 16;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            m_listWidget->item(i)->setSizeHint(QSize(w, 48));
        }
    }
}

void NotesListWindow::updateSyncBadge() {
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

    if (syncActive) {
        m_syncBadge->setText("● Synchronisé");
        m_syncBadge->setStyleSheet("color: #10b981; background: rgba(16, 185, 129, 0.12); padding: 4px 10px; border-radius: 12px; font-size: 11px; font-weight: 600;");
    } else {
        m_syncBadge->setText("○ Local");
        m_syncBadge->setStyleSheet("color: #94a3b8; background: rgba(148, 163, 184, 0.12); padding: 4px 10px; border-radius: 12px; font-size: 11px; font-weight: 500;");
    }
}

void NotesListWindow::applyTheme(bool dark) {
    m_darkTheme = dark;
    m_themeToggleBtn->setIcon(createThemeIcon(dark));
    m_themeToggleBtn->setIconSize(QSize(18, 18));
    m_viewToggleBtn->setIcon(createViewModeIcon(m_isGridView, dark));
    m_mobileSyncBtn->setIcon(createQrIcon(dark));
    updateAutostartButton();

    if (m_updateBtn && !m_hasUpdate) {
        m_updateBtn->setIcon(UpdateManager::createUpdateIcon(false, dark));
    }

    if (dark) {
        setStyleSheet(
            "NotesListWindow { background-color: #0d0f12; color: #f3f4f6; font-family: system-ui, -apple-system, sans-serif; }"
            "QLabel#mainTitle { font-weight: 700; font-size: 16px; color: #ffffff; }"
            "QLabel#subTitle { font-size: 12px; color: #9ca3af; }"
            "QLineEdit#searchEdit { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 0 14px; height: 36px; color: #ffffff; font-size: 13px; }"
            "QLineEdit#searchEdit:focus { border: 1px solid #38bdf8; }"
            "QPushButton#newNoteBtn { background-color: #38bdf8; border: none; border-radius: 8px; padding: 0 16px; height: 36px; color: #0b1320; font-weight: 700; font-size: 13px; }"
            "QPushButton#newNoteBtn::hover { background-color: #7dd3fc; }"
            "QPushButton#autostartBtn { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #ffffff; }"
            "QPushButton#autostartBtn::hover { background-color: #262c36; }"
            "QPushButton#autostartBtn[active=\"true\"] { border: 1px solid rgba(52, 211, 153, 0.45); background-color: rgba(52, 211, 153, 0.1); }"
            "QPushButton#mobileSyncBtn { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #ffffff; }"
            "QPushButton#mobileSyncBtn::hover { background-color: #262c36; }"
            "QPushButton#themeToggleBtn { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #ffffff; }"
            "QPushButton#themeToggleBtn::hover { background-color: #262c36; }"
            "QPushButton#viewToggleBtn { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #ffffff; }"
            "QPushButton#viewToggleBtn::hover { background-color: #262c36; }"
            "QPushButton#updateBtn { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 0 10px; height: 36px; color: #ffffff; font-size: 12px; }"
            "QPushButton#updateBtn::hover { background-color: #262c36; }"
            "QPushButton#filterChip { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 14px; padding: 4px 12px; font-size: 11.5px; color: #9ca3af; }"
            "QPushButton#filterChip::hover { background-color: #262c36; color: #f3f4f6; }"
            "QPushButton#filterChip[active=\"true\"] { background-color: #38bdf8; color: #0b1320; font-weight: 700; border: none; }"
            "QListWidget { background-color: transparent; border: none; outline: none; }"
            "QScrollBar:vertical { width: 8px; background: transparent; margin: 0; }"
            "QScrollBar::handle:vertical { background: rgba(255, 255, 255, 0.15); border-radius: 4px; min-height: 20px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(255, 255, 255, 0.3); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QPushButton#closeBtn { background-color: #16191f; border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 6px; padding: 6px 18px; color: #ffffff; font-size: 12px; }"
            "QPushButton#closeBtn::hover { background-color: #262c36; }"
        );
    } else {
        setStyleSheet(
            "NotesListWindow { background-color: #f8fafc; color: #0f172a; font-family: system-ui, -apple-system, sans-serif; }"
            "QLabel#mainTitle { font-weight: 700; font-size: 16px; color: #0f172a; }"
            "QLabel#subTitle { font-size: 12px; color: #64748b; }"
            "QLineEdit#searchEdit { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; padding: 0 14px; height: 36px; color: #0f172a; font-size: 13px; }"
            "QLineEdit#searchEdit:focus { border: 1px solid #0284c7; }"
            "QPushButton#newNoteBtn { background-color: #0284c7; border: none; border-radius: 8px; padding: 0 16px; height: 36px; color: #ffffff; font-weight: 700; font-size: 13px; }"
            "QPushButton#newNoteBtn::hover { background-color: #0369a1; }"
            "QPushButton#autostartBtn { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #0f172a; }"
            "QPushButton#autostartBtn::hover { background-color: #f1f5f9; }"
            "QPushButton#autostartBtn[active=\"true\"] { border: 1px solid rgba(5, 150, 105, 0.45); background-color: rgba(5, 150, 105, 0.08); }"
            "QPushButton#mobileSyncBtn { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #0f172a; }"
            "QPushButton#mobileSyncBtn::hover { background-color: #f1f5f9; }"
            "QPushButton#themeToggleBtn { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #0f172a; }"
            "QPushButton#themeToggleBtn::hover { background-color: #f1f5f9; }"
            "QPushButton#viewToggleBtn { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; width: 36px; height: 36px; color: #0f172a; }"
            "QPushButton#viewToggleBtn::hover { background-color: #f1f5f9; }"
            "QPushButton#updateBtn { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; padding: 0 10px; height: 36px; color: #0f172a; font-size: 12px; }"
            "QPushButton#updateBtn::hover { background-color: #f1f5f9; }"
            "QPushButton#filterChip { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.08); border-radius: 14px; padding: 4px 12px; font-size: 11.5px; color: #64748b; }"
            "QPushButton#filterChip::hover { background-color: #f1f5f9; color: #0f172a; }"
            "QPushButton#filterChip[active=\"true\"] { background-color: #0284c7; color: #ffffff; font-weight: 700; border: none; }"
            "QListWidget { background-color: transparent; border: none; outline: none; }"
            "QScrollBar:vertical { width: 8px; background: transparent; margin: 0; }"
            "QScrollBar::handle:vertical { background: rgba(0, 0, 0, 0.15); border-radius: 4px; min-height: 20px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(0, 0, 0, 0.3); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QPushButton#closeBtn { background-color: #ffffff; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 6px; padding: 6px 18px; color: #0f172a; font-size: 12px; }"
            "QPushButton#closeBtn::hover { background-color: #f1f5f9; }"
        );
    }

    if (m_hasUpdate && m_updateBtn) {
        m_updateBtn->setStyleSheet("QPushButton#updateBtn { background-color: #0284c7; border: 1px solid #0369a1; border-radius: 8px; padding: 0 12px; height: 36px; color: #ffffff; font-weight: bold; font-size: 12px; } QPushButton#updateBtn::hover { background-color: #0369a1; }");
    }
}

void NotesListWindow::toggleTheme() {
    bool nextTheme = !m_darkTheme;
    applyTheme(nextTheme);
    saveThemeSetting(nextTheme);
    populateList();
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

bool NotesListWindow::loadViewModeSetting() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        file.close();
        return obj.value("view_mode").toString("grid") == "grid";
    }
    return true;
}

void NotesListWindow::saveViewModeSetting(bool isGrid) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";
    QFile file(configPath);
    QJsonObject obj;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        obj = doc.object();
        file.close();
    }
    obj["view_mode"] = isGrid ? "grid" : "list";
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void NotesListWindow::setUpdateAvailable(bool available, const QString& currentHash, const QString& remoteHash, const QString& commitSubject) {
    m_hasUpdate = available;
    m_currentHash = currentHash;
    m_remoteHash = remoteHash;
    m_commitSubject = commitSubject;

    if (m_updateBtn) {
        m_updateBtn->setIcon(UpdateManager::createUpdateIcon(available, m_darkTheme));
        if (available) {
            m_updateBtn->setToolTip(QString("Mise à jour disponible!\nNouveau commit: %1 (%2)").arg(commitSubject, remoteHash));
            m_updateBtn->setText("Mise à jour disponible");
            m_updateBtn->setStyleSheet("QPushButton#updateBtn { background-color: #0284c7; border: 1px solid #0369a1; border-radius: 8px; padding: 0 12px; height: 36px; color: #ffffff; font-weight: bold; font-size: 12px; } QPushButton#updateBtn::hover { background-color: #0369a1; }");
        } else {
            m_updateBtn->setToolTip("L'application est à jour");
            m_updateBtn->setText("");
            applyTheme(m_darkTheme);
        }
    }
}

void NotesListWindow::setUpdateChecking(bool checking) {
    if (m_updateBtn) {
        if (checking) {
            m_updateBtn->setIcon(UpdateManager::createHourglassIcon(m_darkTheme));
            m_updateBtn->setToolTip("Vérification des mises à jour en cours...");
            m_updateBtn->setEnabled(false);
        } else {
            m_updateBtn->setEnabled(true);
            m_updateBtn->setIcon(UpdateManager::createUpdateIcon(m_hasUpdate, m_darkTheme));
        }
    }
}

void NotesListWindow::onUpdateBtnClicked() {
    if (m_hasUpdate) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Mise à jour de Tux-It");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("Une nouvelle version de Tux-It est disponible.");
        msgBox.setInformativeText(QString("Nouveau commit distant :\n\"%1\" (%2)\n\nVoulez-vous télécharger, recompiler et redémarrer Tux-It maintenant ?").arg(m_commitSubject, m_remoteHash));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::Yes) {
            emit applyUpdateRequested();
        }
    } else {
        setUpdateChecking(true);
        emit checkUpdateRequested();
    }
}

void NotesListWindow::updateAutostartButton() {
    if (!m_autostartBtn) return;
    bool enabled = AutostartManager::isEnabled();
    m_autostartBtn->setIcon(AutostartManager::createAutostartIcon(enabled, m_darkTheme));
    m_autostartBtn->setIconSize(QSize(18, 18));
    m_autostartBtn->setProperty("active", enabled);
    m_autostartBtn->style()->unpolish(m_autostartBtn);
    m_autostartBtn->style()->polish(m_autostartBtn);

    if (enabled) {
        m_autostartBtn->setToolTip("Démarrage automatique : Activé (Cliquer pour désactiver)");
    } else {
        m_autostartBtn->setToolTip("Démarrage automatique : Désactivé (Cliquer pour lancer Tux-It au démarrage)");
    }
}

void NotesListWindow::toggleAutostart() {
    AutostartManager::toggle();
    updateAutostartButton();
}
