#include "setupwizard.h"
#include "stickywindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>

SetupWizard::SetupWizard(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Configuration Initiale - Tux-It");
    setWindowIcon(StickyWindow::createFoxIcon());
    resize(480, 420);
    
    setupUi();
}

SetupWizard::~SetupWizard() {}

void SetupWizard::setupUi() {
    // Style moderne premium sombre
    setStyleSheet(
        "SetupWizard { background-color: #1e1e24; color: #ffffff; }"
        "QLabel { color: #ffffff; font-size: 12px; }"
        "QLabel#title { font-weight: bold; font-size: 18px; color: #ffffff; }"
        "QCheckBox { color: #ffffff; font-size: 12px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QLineEdit { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 6px; padding: 6px 10px; color: #ffffff; }"
        "QLineEdit:focus { border: 1px solid #90CAF9; }"
        "QPushButton { background-color: #2b2b36; border: 1px solid #3f3f50; border-radius: 6px; padding: 8px 16px; color: #ffffff; font-weight: bold; }"
        "QPushButton::hover { background-color: #3f3f50; }"
        "QPushButton#finishBtn { background-color: #1976D2; border: none; }"
        "QPushButton#finishBtn::hover { background-color: #1565C0; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // En-tête avec Icône de l'application
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* logoLabel = new QLabel(this);
    logoLabel->setPixmap(StickyWindow::createFoxIcon().pixmap(64, 64));
    logoLabel->setFixedSize(64, 64);
    
    QVBoxLayout* headerTextLayout = new QVBoxLayout();
    QLabel* titleLabel = new QLabel("Bienvenue sur Tux-It !", this);
    titleLabel->setObjectName("title");
    QLabel* subtitleLabel = new QLabel("Assistant de configuration rapide", this);
    subtitleLabel->setStyleSheet("color: #aaaaaa; font-size: 11px;");
    headerTextLayout->addWidget(titleLabel);
    headerTextLayout->addWidget(subtitleLabel);

    headerLayout->addWidget(logoLabel);
    headerLayout->addLayout(headerTextLayout);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Étape 1 : Raccourci Bureau
    QLabel* step1Title = new QLabel("<b>Étape 1 : Raccourci système</b>", this);
    mainLayout->addWidget(step1Title);

    m_shortcutCheckbox = new QCheckBox("Créer un raccourci de bureau (Lancement via le menu d'applications)", this);
    m_shortcutCheckbox->setChecked(true);
    mainLayout->addWidget(m_shortcutCheckbox);

    m_autostartCheckbox = new QCheckBox("Lancer automatiquement au démarrage de la session (Autostart)", this);
    m_autostartCheckbox->setChecked(true);
    mainLayout->addWidget(m_autostartCheckbox);

    // Séparateur
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("background-color: #3f3f50; max-height: 1px;");
    mainLayout->addWidget(line);

    // Étape 2 : Synchronisation en ligne
    QLabel* step2Title = new QLabel("<b>Étape 2 : Synchronisation (Facultatif)</b>", this);
    mainLayout->addWidget(step2Title);

    m_syncCheckbox = new QCheckBox("Activer la synchronisation en ligne", this);
    mainLayout->addWidget(m_syncCheckbox);

    // Widget conteneur pour la config du serveur (masqué par défaut)
    m_syncConfigWidget = new QWidget(this);
    QVBoxLayout* syncLayout = new QVBoxLayout(m_syncConfigWidget);
    syncLayout->setContentsMargins(16, 0, 0, 0);
    syncLayout->setSpacing(8);

    QLabel* urlLabel = new QLabel("URL du serveur de synchronisation :", m_syncConfigWidget);
    m_serverUrlEdit = new QLineEdit(m_syncConfigWidget);
    m_serverUrlEdit->setPlaceholderText("Exemple : http://localhost:8282");

    QLabel* keyLabel = new QLabel("Clé d'API (Bearer Token) :", m_syncConfigWidget);
    m_apiKeyEdit = new QLineEdit(m_syncConfigWidget);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("Saisissez votre clé d'authentification");

    syncLayout->addWidget(urlLabel);
    syncLayout->addWidget(m_serverUrlEdit);
    syncLayout->addWidget(keyLabel);
    syncLayout->addWidget(m_apiKeyEdit);
    
    m_syncConfigWidget->setVisible(false);
    mainLayout->addWidget(m_syncConfigWidget);

    mainLayout->addStretch();

    // Boutons d'actions
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton("Annuler", this);
    m_finishButton = new QPushButton("Terminer", this);
    m_finishButton->setObjectName("finishBtn");
    m_finishButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setCursor(Qt::PointingHandCursor);

    btnLayout->addWidget(m_cancelButton);
    btnLayout->addStretch();
    btnLayout->addWidget(m_finishButton);
    mainLayout->addLayout(btnLayout);

    // Connexions
    connect(m_syncCheckbox, &QCheckBox::toggled, m_syncConfigWidget, &QWidget::setVisible);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(m_finishButton, &QPushButton::clicked, this, [this]() {
        if (m_syncCheckbox->isChecked()) {
            QString url = m_serverUrlEdit->text().trimmed();
            QString key = m_apiKeyEdit->text().trimmed();
            
            if (url.isEmpty() || key.isEmpty()) {
                QMessageBox::warning(this, "Champs manquants", "Veuillez remplir l'URL du serveur et la clé d'API si la synchronisation est activée.");
                return;
            }
            
            // Indiquer visuellement que la vérification est en cours
            setCursor(Qt::WaitCursor);
            m_finishButton->setEnabled(false);
            m_finishButton->setText("Vérification...");
            
            QString errorMsg;
            bool ok = validateSyncConfig(url, key, errorMsg);
            
            unsetCursor();
            m_finishButton->setEnabled(true);
            m_finishButton->setText("Terminer");
            
            if (!ok) {
                QMessageBox::critical(this, "Erreur de connexion", errorMsg);
                return;
            }
        }
        
        // Exécuter la création du raccourci si sélectionné
        if (m_shortcutCheckbox->isChecked()) {
            createDesktopLauncher();
        }
        
        accept();
    });
}

void SetupWizard::createDesktopLauncher() {
    QString homePath = QDir::homePath();
    
    // 1. Exporter l'icône de bureau
    QString iconDir = homePath + "/.local/share/icons";
    QDir().mkpath(iconDir);
    QString iconPath = iconDir + "/tux-it.png";
    
    QIcon icon = StickyWindow::createFoxIcon();
    QPixmap pixmap = icon.pixmap(256, 256);
    pixmap.save(iconPath);

    // 2. S'assurer que le dossier des applications existe
    QString appDir = homePath + "/.local/share/applications";
    QDir().mkpath(appDir);
    QString desktopPath = appDir + "/tux-it.desktop";

    // 3. Écrire le fichier desktop
    QFile file(desktopPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n"
            << "Name=Tux-It\n"
            << "Comment=Vos post-its mignons sur le bureau\n"
            // Comme le binaire principal sera copié dans ~/.local/bin/tux-it par l'installateur
            << "Exec=" << homePath << "/.local/bin/tux-it\n"
            << "Icon=" << iconPath << "\n"
            << "Terminal=false\n"
            << "Type=Application\n"
            << "Categories=Utility;Office;\n"
            << "StartupNotify=true\n";
        file.close();
        
        // Permissions
        QFile::setPermissions(desktopPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                                           QFile::ReadGroup | QFile::ReadOther);

        // 4. Si sélectionné, créer également le fichier d'autostart
        if (m_autostartCheckbox->isChecked()) {
            QString autostartDir = homePath + "/.config/autostart";
            QDir().mkpath(autostartDir);
            QString autostartPath = autostartDir + "/tux-it.desktop";

            QFile autostartFile(autostartPath);
            if (autostartFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream outAuto(&autostartFile);
                outAuto << "[Desktop Entry]\n"
                        << "Name=Tux-It\n"
                        << "Comment=Vos post-its mignons sur le bureau\n"
                        << "Exec=" << homePath << "/.local/bin/tux-it\n"
                        << "Icon=" << iconPath << "\n"
                        << "Terminal=false\n"
                        << "Type=Application\n"
                        << "Categories=Utility;Office;\n"
                        << "StartupNotify=true\n";
                autostartFile.close();
                QFile::setPermissions(autostartPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                                                     QFile::ReadGroup | QFile::ReadOther);
            }
        }
    }
}

bool SetupWizard::isSyncEnabled() const {
    return m_syncCheckbox->isChecked();
}

QString SetupWizard::serverUrl() const {
    return m_serverUrlEdit->text().trimmed();
}

QString SetupWizard::apiKey() const {
    return m_apiKeyEdit->text().trimmed();
}

bool SetupWizard::createDesktopShortcut() const {
    return m_shortcutCheckbox->isChecked();
}

bool SetupWizard::runAtStartup() const {
    return m_autostartCheckbox->isChecked();
}

bool SetupWizard::validateSyncConfig(const QString& serverUrl, const QString& apiKey, QString& errorMessage) {
    QNetworkAccessManager manager;
    QUrl url(serverUrl.trimmed() + "/notes");
    if (!url.isValid()) {
        errorMessage = "L'URL saisie n'est pas valide.";
        return false;
    }
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey.trimmed().toUtf8());
    
    // Timeout de 5 secondes pour éviter de bloquer indéfiniment
    request.setTransferTimeout(5000);
    
    QNetworkReply* reply = manager.get(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        reply->deleteLater();
        return true;
    } else {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            errorMessage = "La clé d'API saisie est invalide ou non autorisée par le serveur.";
        } else {
            errorMessage = QString("Impossible de contacter le serveur. Détail : %1\nVeuillez vérifier l'adresse du serveur et votre connexion réseau.")
                            .arg(reply->errorString());
        }
        reply->deleteLater();
        return false;
    }
}
