#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

class SetupWizard : public QDialog {
    Q_OBJECT
public:
    explicit SetupWizard(QWidget* parent = nullptr);
    ~SetupWizard() override;

    bool isSyncEnabled() const;
    QString serverUrl() const;
    QString apiKey() const;
    bool createDesktopShortcut() const;
    bool runAtStartup() const;

private:
    void setupUi();
    void createDesktopLauncher();
    bool validateSyncConfig(const QString& serverUrl, const QString& apiKey, QString& errorMessage);

    QCheckBox* m_shortcutCheckbox;
    QCheckBox* m_autostartCheckbox;
    QCheckBox* m_syncCheckbox;
    QWidget* m_syncConfigWidget;
    QLineEdit* m_serverUrlEdit;
    QLineEdit* m_apiKeyEdit;
    
    QPushButton* m_cancelButton;
    QPushButton* m_finishButton;
};

#endif // SETUPWIZARD_H
