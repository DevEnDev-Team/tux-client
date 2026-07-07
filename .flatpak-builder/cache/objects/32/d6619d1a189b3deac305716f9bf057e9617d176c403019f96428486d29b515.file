#include "syncqrcodedialog.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>

SyncQrCodeDialog::SyncQrCodeDialog(const QString& serverUrl, const QString& apiKey, QWidget* parent)
    : QDialog(parent) {
    
    setWindowTitle("Synchronisation Mobile");
    setFixedSize(360, 420);
    
    // Style de la fenêtre (Thème sombre premium violet/indigo)
    setStyleSheet(
        "QDialog {"
        "  background-color: #17171e;"
        "  color: #ffffff;"
        "}"
        "QLabel {"
        "  color: #ffffff;"
        "  font-family: 'Outfit', sans-serif;"
        "}"
        "QPushButton {"
        "  background-color: #7c4dff;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 10px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #651fff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #536dfe;"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    QLabel* titleLabel = new QLabel("Synchronisation Tux-It Mobile", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #b388ff;");
    layout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(
        "Ouvrez la PWA Tux-It sur votre téléphone, allez dans la configuration et flashez ce QR Code pour importer vos paramètres de synchronisation.", 
        this
    );
    descLabel->setWordWrap(true);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet("font-size: 12px; color: #90a4ae; line-height: 1.4;");
    layout->addWidget(descLabel);

    // Conteneur du QR Code
    m_qrLabel = new QLabel(this);
    m_qrLabel->setFixedSize(220, 220);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setStyleSheet(
        "background-color: #1c1c24;"
        "border: 1px solid #2b2b36;"
        "border-radius: 12px;"
    );
    
    QHBoxLayout* qrLayout = new QHBoxLayout();
    qrLayout->addWidget(m_qrLabel, 0, Qt::AlignCenter);
    layout->addLayout(qrLayout);

    m_statusLabel = new QLabel("Génération du QR Code...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 11px; color: #ff9800;");
    layout->addWidget(m_statusLabel);

    QPushButton* closeButton = new QPushButton("Fermer", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);

    m_networkManager = new QNetworkAccessManager(this);
    loadQrCode(serverUrl, apiKey);
}

SyncQrCodeDialog::~SyncQrCodeDialog() {
}

void SyncQrCodeDialog::loadQrCode(const QString& serverUrl, const QString& apiKey) {
    // Encoder les informations de synchronisation en JSON
    QJsonObject syncObj;
    syncObj["url"] = serverUrl;
    syncObj["key"] = apiKey;

    QJsonDocument doc(syncObj);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    // Préparer l'URL pour l'API de QR Code publique
    QString apiUrl = "https://api.qrserver.com/v1/create-qr-code/";
    QUrl url(apiUrl);
    QUrlQuery query;
    query.addQueryItem("size", "220x220");
    query.addQueryItem("color", "7c4dff"); // Assorti au violet de l'application
    query.addQueryItem("bgcolor", "1c1c24"); // Fond assorti
    query.addQueryItem("data", jsonString);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pixmap;
            if (pixmap.loadFromData(data)) {
                m_qrLabel->setPixmap(pixmap);
                m_statusLabel->setText("QR Code chargé avec succès !");
                m_statusLabel->setStyleSheet("font-size: 11px; color: #4caf50;");
            } else {
                m_statusLabel->setText("Erreur d'interprétation de l'image");
                m_statusLabel->setStyleSheet("font-size: 11px; color: #f44336;");
            }
        } else {
            m_statusLabel->setText("Erreur de téléchargement : " + reply->errorString());
            m_statusLabel->setStyleSheet("font-size: 11px; color: #f44336;");
        }
        reply->deleteLater();
    });
}
