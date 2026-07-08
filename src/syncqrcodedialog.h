#ifndef SYNCQRCODEDIALOG_H
#define SYNCQRCODEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class SyncQrCodeDialog : public QDialog {
    Q_OBJECT

public:
    SyncQrCodeDialog(const QString& serverUrl, const QString& apiKey, QWidget* parent = nullptr);
    ~SyncQrCodeDialog();

private:
    void loadQrCode(const QString& serverUrl, const QString& apiKey);

    QLabel* m_qrLabel;
    QLabel* m_statusLabel;
    QNetworkAccessManager* m_networkManager;
};

#endif // SYNCQRCODEDIALOG_H
