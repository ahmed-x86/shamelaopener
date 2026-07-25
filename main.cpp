#include "theme.h"
#include <QApplication>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QTimer>
#include <QMouseEvent>
#include <QFontDatabase>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QProcess>
#include <QTextStream>
#include <QDir>
#include <QFileInfo> 
#include <QStringList>

class DesktopIconChoiceDialog : public QDialog {
    Q_OBJECT
public:
    enum Choice { Shamela, App, Cancel };
    Choice resultChoice = Cancel;

    explicit DesktopIconChoiceDialog(Language lang, QWidget* parent = nullptr) : QDialog(parent) {
        bool isAr = (lang == Language::Arabic);
        setWindowTitle(isAr ? "إنشاء أيقونة سطح المكتب" : "Create Desktop Icon");
        setFixedSize(350, 160);
        setLayoutDirection(isAr ? Qt::RightToLeft : Qt::LeftToRight);

        auto* layout = new QVBoxLayout(this);
        
        auto* lbl = new QLabel(isAr ? "لأي برنامج تود إنشاء أيقونة؟" : "For which program do you want to create an icon?", this);
        lbl->setAlignment(Qt::AlignCenter);
        layout->addWidget(lbl);
        
        auto* btnShamela = new QPushButton(isAr ? "للشاملة" : "For Shamela", this);
        btnShamela->setCursor(Qt::PointingHandCursor);
        btnShamela->setFixedHeight(40);

        auto* btnApp = new QPushButton(isAr ? "لهذا البرنامج" : "For this program", this);
        btnApp->setCursor(Qt::PointingHandCursor);
        btnApp->setFixedHeight(40);
        
        layout->addWidget(btnShamela);
        layout->addWidget(btnApp);
        
        connect(btnShamela, &QPushButton::clicked, this, [this](){
            resultChoice = Shamela;
            accept();
        });
        connect(btnApp, &QPushButton::clicked, this, [this](){
            resultChoice = App;
            accept();
        });
    }
};

class DownloadChoiceDialog : public QDialog {
    Q_OBJECT
public:
    enum Choice { Manual, Auto, Cancel };
    Choice resultChoice = Cancel;

    explicit DownloadChoiceDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("كيف تود أن تحملها؟");
        setFixedSize(350, 150);
        setLayoutDirection(Qt::RightToLeft);

        auto* layout = new QVBoxLayout(this);
        
        auto* lbl = new QLabel("اختر طريقة تحميل المكتبة الشاملة:", this);
        lbl->setAlignment(Qt::AlignCenter);
        layout->addWidget(lbl);
        
        auto* btnManual = new QPushButton("يدوياً (عبر المتصفح)", this);
        btnManual->setCursor(Qt::PointingHandCursor);
        btnManual->setFixedHeight(40);

        auto* btnAuto = new QPushButton("تحميل آلي هنا", this);
        btnAuto->setCursor(Qt::PointingHandCursor);
        btnAuto->setFixedHeight(40);
        
        layout->addWidget(btnManual);
        layout->addWidget(btnAuto);
        
        connect(btnManual, &QPushButton::clicked, this, [this](){
            resultChoice = Manual;
            accept();
        });
        connect(btnAuto, &QPushButton::clicked, this, [this](){
            resultChoice = Auto;
            accept();
        });
    }
};

class DownloadProgressDialog : public QDialog {
    Q_OBJECT
public:
    DownloadProgressDialog(const QString& savePath, QWidget* parent = nullptr) 
        : QDialog(parent), m_savePath(savePath) {
        
        setWindowTitle("جاري التحميل...");
        setFixedSize(450, 200);
        setLayoutDirection(Qt::RightToLeft);

        auto* layout = new QVBoxLayout(this);
        
        m_lblStatus = new QLabel("جاري الاتصال بالخادم...", this);
        m_lblStatus->setAlignment(Qt::AlignCenter);
        
        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 100);
        m_progressBar->setFixedHeight(25);
        
        m_lblSize = new QLabel("الحجم: 0 MB / 0 MB", this);
        m_lblSpeed = new QLabel("السرعة: 0 KB/s", this);
        
        auto* infoLayout = new QHBoxLayout();
        infoLayout->addWidget(m_lblSize);
        infoLayout->addStretch();
        infoLayout->addWidget(m_lblSpeed);
        
        layout->addWidget(m_lblStatus);
        layout->addWidget(m_progressBar);
        layout->addLayout(infoLayout);
        
        m_file = new QFile(m_savePath, this);
        if (!m_file->open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "خطأ", "لا يمكن فتح مسار الملف للكتابة!");
            QTimer::singleShot(0, this, &QDialog::reject);
            return;
        }

        m_manager = new QNetworkAccessManager(this);
        QNetworkRequest request(QUrl("https://archive.org/download/shamela_download/shamela-linux-1447.11.tar.xz"));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        
        m_reply = m_manager->get(request);
        
        connect(m_reply, &QNetworkReply::readyRead, this, &DownloadProgressDialog::onReadyRead);
        connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadProgressDialog::onDownloadProgress);
        connect(m_reply, &QNetworkReply::finished, this, &DownloadProgressDialog::onFinished);
        
        m_timer.start();
    }

private slots:
    void onReadyRead() {
        if (m_file && m_reply) {
            m_file->write(m_reply->readAll());
        }
    }
    
    void onDownloadProgress(qint64 bytesRead, qint64 totalBytes) {
        if (totalBytes > 0) {
            m_progressBar->setMaximum(totalBytes);
            m_progressBar->setValue(bytesRead);
            
            double downloadedMB = bytesRead / (1024.0 * 1024.0);
            double totalMB = totalBytes / (1024.0 * 1024.0);
            m_lblSize->setText(QString::asprintf("الحجم: %.2f MB / %.2f MB", downloadedMB, totalMB));
            
            qint64 elapsedMs = m_timer.elapsed();
            if (elapsedMs > 0) {
                double speedKBps = (bytesRead / 1024.0) / (elapsedMs / 1000.0);
                if (speedKBps > 1024) {
                    m_lblSpeed->setText(QString::asprintf("السرعة: %.2f MB/s", speedKBps / 1024.0));
                } else {
                    m_lblSpeed->setText(QString::asprintf("السرعة: %.2f KB/s", speedKBps));
                }
            }
            m_lblStatus->setText("جاري تحميل الشاملة...");
        }
    }
    
    void onFinished() {
        if (m_file) {
            m_file->close();
        }
        if (m_reply->error() == QNetworkReply::NoError) {
            m_lblStatus->setText("تم التحميل بنجاح!");
            QMessageBox::information(this, "نجاح", "تم تحميل الشاملة بنجاح إلى المسار المحدد.");
            accept();
        } else {
            m_lblStatus->setText("حدث خطأ أثناء التحميل.");
            QMessageBox::critical(this, "خطأ", "فشل التحميل:\n" + m_reply->errorString());
            reject();
        }
        m_reply->deleteLater();
    }

private:
    QString m_savePath;
    QFile* m_file{};
    QNetworkAccessManager* m_manager{};
    QNetworkReply* m_reply{};
    QElapsedTimer m_timer;
    
    QLabel* m_lblStatus{};
    QLabel* m_lblSpeed{};
    QLabel* m_lblSize{};
    QProgressBar* m_progressBar{};
};

class DependenciesCheckDialog : public QDialog {
    Q_OBJECT
public:
    explicit DependenciesCheckDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("فحص الاعتماديات");
        setFixedSize(450, 420);
        setLayoutDirection(Qt::LeftToRight); 

        auto* layout = new QVBoxLayout(this);
        
        auto* lblTitle = new QLabel("<b>حالة تبعيات المكتبة الشاملة:</b>", this);
        lblTitle->setAlignment(Qt::AlignCenter);
        lblTitle->setStyleSheet("font-size: 16px; margin-bottom: 10px;");
        layout->addWidget(lblTitle);

        QList<QStringList> dependencies = {
            {"fontconfig", "fontconfig-git", "fontconfig-ubuntu"},
            {"freetype2", "freetype2-qdoled-aw3225qf", "freetype2-qdoled-gen3", "freetype2-git", "freetype2-macos", "freetype2-qdoled", "freetype2-woled"},
            {"glibc", "glibc-git", "glibc-git-native-pgo", "glibc-eac"},
            {"hicolor-icon-theme", "hicolor-icon-theme-git"},
            {"libselinux"},
            {"zlib", "zlib-git", "zlib-ng-compat-git", "zlib-ng-compat"}
        };

        for (const auto& group : dependencies) {
            auto* rowLayout = new QHBoxLayout();
            
            QLabel* lblName = new QLabel(group.first(), this);
            lblName->setStyleSheet("font-weight: bold; font-size: 14px;");
            
            QLabel* lblStatus = new QLabel("Checking...", this);
            lblStatus->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            
            rowLayout->addWidget(lblName);
            rowLayout->addStretch();
            rowLayout->addWidget(lblStatus);
            
            layout->addLayout(rowLayout);

            bool installed = false;
            for (const QString& pkg : group) {
                QProcess process;
                process.start("pacman", QStringList() << "-Qq" << pkg);
                process.waitForFinished(1000); 
                if (process.exitCode() == 0) {
                    installed = true;
                    break;
                }
            }

            if (installed) {
                lblStatus->setText("install");
                lblStatus->setStyleSheet("color: #a6e3a1; font-weight: bold; font-size: 14px;"); 
            } else {
                lblStatus->setText("not install");
                lblStatus->setStyleSheet("color: #f38ba8; font-weight: bold; font-size: 14px;"); 
            }
        }
        
        layout->addStretch();
        
        auto* btnClose = new QPushButton("إغلاق", this);
        btnClose->setCursor(Qt::PointingHandCursor);
        btnClose->setFixedHeight(40);
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(btnClose);
    }
};

class RippleButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal rippleRadius READ rippleRadius WRITE setRippleRadius)
    Q_PROPERTY(qreal rippleOpacity READ rippleOpacity WRITE setRippleOpacity)

public:
    explicit RippleButton(QWidget* parent = nullptr) : QPushButton(parent) {
        setCursor(Qt::PointingHandCursor);
    }
    qreal rippleRadius() const { return m_rippleRadius; }
    void setRippleRadius(qreal r) { m_rippleRadius = r; update(); }
    qreal rippleOpacity() const { return m_rippleOpacity; }
    void setRippleOpacity(qreal o) { m_rippleOpacity = o; update(); }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QPushButton::mousePressEvent(event);
        m_ripplePos = event->position();
        m_rippleRadius = 0;
        m_rippleOpacity = 0.2;

        auto* radiusAnim = new QPropertyAnimation(this, "rippleRadius");
        radiusAnim->setDuration(350);
        radiusAnim->setStartValue(0);
        radiusAnim->setEndValue(width() * 1.5);
        radiusAnim->setEasingCurve(QEasingCurve::OutQuad);

        auto* opacityAnim = new QPropertyAnimation(this, "rippleOpacity");
        opacityAnim->setDuration(350);
        opacityAnim->setStartValue(0.2);
        opacityAnim->setEndValue(0.0);

        auto* group = new QParallelAnimationGroup(this);
        group->addAnimation(radiusAnim);
        group->addAnimation(opacityAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void paintEvent(QPaintEvent* event) override {
        QPushButton::paintEvent(event);
        if (m_rippleRadius > 0 && m_rippleOpacity > 0) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(rect(), 8, 8);
            painter.setClipPath(path);
            painter.setBrush(QColor(0, 0, 0, static_cast<int>(m_rippleOpacity * 255)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(m_ripplePos, m_rippleRadius, m_rippleRadius);
        }
    }

private:
    QPointF m_ripplePos;
    qreal m_rippleRadius = 0;
    qreal m_rippleOpacity = 0;
};

class GlowWindow : public QWidget {
    Q_OBJECT

public:
    explicit GlowWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(500, 460); 
        m_currentGlowPos = QPointF(width() / 2.0, height() / 2.0);
        m_targetGlowPos  = m_currentGlowPos;

        buildMainLayout();
        buildSidebar();

        m_glowTimer = new QTimer(this);
        m_glowTimer->setInterval(16);
        connect(m_glowTimer, &QTimer::timeout, this, [this]() {
            m_currentGlowPos += (m_targetGlowPos - m_currentGlowPos) * 0.08;
            update();
        });
        m_glowTimer->start();

        qApp->installEventFilter(this);
        
        loadSettings();
        if (m_isCustomTheme) {
            applyCustomTheme(m_colors);
        } else {
            applyTheme(m_theme);
        }
        setLanguage(m_currentLang);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(m_colors.Base));

        QRadialGradient glow(m_currentGlowPos, 240.0);
        QColor glowColor(m_colors.Accent3);
        
        int alpha = (m_colors.isDark) ? 30 : 80;
        glowColor.setAlpha(alpha);
        glow.setColorAt(0.0, glowColor);
        glowColor.setAlpha(0);
        glow.setColorAt(1.0, glowColor);

        painter.setBrush(QBrush(glow));
        painter.setPen(Qt::NoPen);
        painter.drawRect(rect());
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::MouseMove) {
            m_targetGlowPos = mapFromGlobal(static_cast<QMouseEvent*>(event)->globalPosition().toPoint());
        }
        if (event->type() == QEvent::MouseButtonPress && m_isSidebarOpen) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->pos().x() > 250) { 
                toggleSidebar();
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        if (m_sidebar) {
            if (m_isSidebarOpen) {
                m_sidebar->setGeometry(0, 0, 250, height());
            } else {
                m_sidebar->setGeometry(-250, 0, 250, height());
            }
        }
    }

private:
    void buildMainLayout() {
        m_mainContent = new QWidget(this);
        auto* root = new QVBoxLayout(m_mainContent);
        root->setContentsMargins(24, 18, 24, 18);
        root->setSpacing(12);

        auto* windowLayout = new QVBoxLayout(this);
        windowLayout->setContentsMargins(0, 0, 0, 0);
        windowLayout->addWidget(m_mainContent);

        m_topBar = new QWidget(this);
        m_topBar->setLayoutDirection(Qt::LeftToRight);
        auto* topRow = new QHBoxLayout(m_topBar);
        topRow->setContentsMargins(0, 0, 0, 0);

        m_hamburgerBtn = new QPushButton("☰");
        m_hamburgerBtn->setObjectName("hamburgerBtn");
        m_hamburgerBtn->setCursor(Qt::PointingHandCursor);
        m_hamburgerBtn->setFont(QFont("Segoe UI", 18, QFont::Bold));
        
        connect(m_hamburgerBtn, &QPushButton::clicked, this, &GlowWindow::toggleSidebar);

        topRow->addWidget(m_hamburgerBtn);
        topRow->addStretch();
        root->addWidget(m_topBar);

        m_stackedWidget = new QStackedWidget(this);

        m_mainMenuPage = new QWidget(this);
        auto* mainLayout = new QVBoxLayout(m_mainMenuPage);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(12);

        mainLayout->addStretch();
        m_btnOpen = new RippleButton(); 
        m_btnOpen->setObjectName("btnOpen"); 
        m_btnOpen->setFixedSize(250, 48); 
        
        connect(m_btnOpen, &QPushButton::clicked, this, [this]() {
            if (!m_shamelaPath.isEmpty() && QFile::exists(m_shamelaPath)) {
                QFileInfo fileInfo(m_shamelaPath);
                QProcess::startDetached(m_shamelaPath, QStringList(), fileInfo.absolutePath());
            } else {
                QMessageBox::information(this, "تنبيه", "لم يتم العثور على المسار أو لم يتم تحديده بعد.\nيرجى تثبيتها أو تحديد مسار ملف launch.sh من إعدادات الشاملة.");
            }
        });

        mainLayout->addWidget(m_btnOpen, 0, Qt::AlignCenter);
        mainLayout->addStretch();
        m_stackedWidget->addWidget(m_mainMenuPage);

        m_shamelaSettingsPage = new QWidget(this);
        auto* shamelaSettingsLayout = new QVBoxLayout(m_shamelaSettingsPage);
        shamelaSettingsLayout->setContentsMargins(0, 0, 0, 0);
        shamelaSettingsLayout->setSpacing(12);

        m_btnDownload = new RippleButton(); 
        m_btnDownload->setObjectName("btnDownload"); 
        m_btnDownload->setFixedHeight(48);
        
        connect(m_btnDownload, &QPushButton::clicked, this, [this]() {
            DownloadChoiceDialog choiceDlg(this);
            choiceDlg.setStyleSheet(this->styleSheet()); 
            
            if (choiceDlg.exec() == QDialog::Accepted) {
                if (choiceDlg.resultChoice == DownloadChoiceDialog::Manual) {
                    QDesktopServices::openUrl(QUrl("https://shamela.ws/page/download"));
                } else if (choiceDlg.resultChoice == DownloadChoiceDialog::Auto) {
                    QString savePath = QFileDialog::getSaveFileName(
                        nullptr, "أين تود حفظ الشاملة؟", "shamela-linux-1447.11.tar.xz", "Archives (*.tar.xz)"
                    );
                    if (!savePath.isEmpty()) {
                        DownloadProgressDialog progressDlg(savePath, this);
                        progressDlg.setStyleSheet(this->styleSheet());
                        progressDlg.exec();
                    }
                }
            }
        });

        shamelaSettingsLayout->addWidget(m_btnDownload);

        m_btnCheckDeps = new RippleButton();
        m_btnCheckDeps->setObjectName("btnCheckDeps");
        m_btnCheckDeps->setFixedHeight(48);
        
        connect(m_btnCheckDeps, &QPushButton::clicked, this, [this]() {
            DependenciesCheckDialog depsDlg(this);
            depsDlg.setStyleSheet(this->styleSheet());
            depsDlg.exec();
        });
        
        shamelaSettingsLayout->addWidget(m_btnCheckDeps);

        auto* midRow = new QHBoxLayout();
        m_btnArchive = new RippleButton(); m_btnArchive->setObjectName("btnArchive");
        m_btnFolder  = new RippleButton(); m_btnFolder->setObjectName("btnFolder");
        m_btnArchive->setFixedHeight(48); m_btnFolder->setFixedHeight(48);
        
        connect(m_btnFolder, &QPushButton::clicked, this, [this]() {
            QString filePath = QFileDialog::getOpenFileName(nullptr, "اختر ملف التشغيل (مثلاً launch.sh)", QDir::homePath(), "Executables/Scripts (*.sh);;All Files (*)");
            if (!filePath.isEmpty()) {
                m_shamelaPath = filePath;
                saveSettings(); 
                QMessageBox::information(this, "نجاح", "تم حفظ مسار ملف التشغيل بنجاح لتشغيله لاحقاً:\n" + filePath);
            }
        });

        connect(m_btnArchive, &QPushButton::clicked, this, [this]() {
            QString archivePath = QFileDialog::getOpenFileName(nullptr, "اختر ملف الشاملة المضغوط", "", "Archives (*.tar.xz *.tar.gz *.tar *.zip);;All Files (*)");
            if (archivePath.isEmpty()) return;

            QString extractPath = QFileDialog::getExistingDirectory(nullptr, "أين تود التثبيت (فك الضغط)؟", "");
            if (extractPath.isEmpty()) return;

            QDialog* extractDlg = new QDialog(this);
            extractDlg->setWindowTitle("جاري التثبيت...");
            extractDlg->setFixedSize(400, 150);
            extractDlg->setStyleSheet(this->styleSheet());
            extractDlg->setLayoutDirection(Qt::RightToLeft);
            extractDlg->setAttribute(Qt::WA_DeleteOnClose);

            auto* layout = new QVBoxLayout(extractDlg);
            auto* lbl = new QLabel("جاري فك الضغط، يرجى الانتظار...\nقد تستغرق العملية بعض الوقت.", extractDlg);
            lbl->setAlignment(Qt::AlignCenter);
            layout->addWidget(lbl);

            auto* progressBar = new QProgressBar(extractDlg);
            progressBar->setRange(0, 0);
            layout->addWidget(progressBar);

            QProcess* process = new QProcess(extractDlg);
            
            if (archivePath.endsWith(".zip", Qt::CaseInsensitive)) {
                process->setProgram("unzip");
                process->setArguments(QStringList() << "-o" << archivePath << "-d" << extractPath);
            } else {
                process->setProgram("tar");
                process->setArguments(QStringList() << "-xf" << archivePath << "-C" << extractPath);
            }

            connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [this, extractDlg, extractPath](int exitCode, QProcess::ExitStatus exitStatus) {
                
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    QDir dir(extractPath);
                    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
                    QStringList subDirs = dir.entryList();
                    QString targetPath = extractPath;

                    if (subDirs.size() == 1) {
                        targetPath = dir.absoluteFilePath(subDirs.first());
                    } else if (subDirs.contains("shamela", Qt::CaseInsensitive)) {
                        targetPath = dir.absoluteFilePath("shamela");
                    }
                    
                    QString scriptPath = targetPath + "/launch.sh";
                    QFile scriptFile(scriptPath);
                    
                    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&scriptFile);
                        QString scriptContent = R"EOF(#!/bin/bash

BASE_DIR="$(dirname "$(realpath "$0")")"
APP_BIN_DIR="$BASE_DIR/app/linux/64/bin"

cd "$BASE_DIR" || exit

exec env -u XDG_CURRENT_DESKTOP \
         -u XDG_SESSION_DESKTOP \
         -u DESKTOP_SESSION \
         -u KDE_FULL_SESSION \
         -u KDE_SESSION_VERSION \
         -u GNOME_DESKTOP_SESSION_ID \
         -u MATE_DESKTOP_SESSION_ID \
         -u CINNAMON_VERSION \
         -u LXQT_SESSION_VERSION \
         LD_LIBRARY_PATH="$APP_BIN_DIR:$LD_LIBRARY_PATH" \
         QT_API=pyside2 \
         QT_QPA_PLATFORM=xcb \
         QT_STYLE_OVERRIDE="fusion" \
         QT_QPA_PLATFORMTHEME="" \
         PYTHONUTF8=1 \
         PYTHONIOENCODING=utf-8 \
         LANG=C.UTF-8 \
         LC_ALL=C.UTF-8 \
         "$APP_BIN_DIR/shamela" "$@"
)EOF";
                        out << scriptContent;
                        scriptFile.close();
                        scriptFile.setPermissions(scriptFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
                        
                        m_shamelaPath = scriptPath;
                        saveSettings(); 
                        
                        extractDlg->accept();
                        QMessageBox::information(this, "نجاح", "تم التثبيت وفك الضغط، وتم تسجيل مسار التشغيل بنجاح:\n" + scriptPath);
                    } else {
                        extractDlg->accept();
                        QMessageBox::warning(this, "تحذير", "تم فك الضغط بنجاح، لكن فشل إنشاء ملف launch.sh (تأكد من صلاحيات المجلد).");
                    }
                } else {
                    extractDlg->reject();
                    QMessageBox::critical(this, "خطأ", "فشلت عملية فك الضغط! تأكد من سلامة ملف الأرشيف.");
                }
            });

            process->start();
            extractDlg->setModal(true);
            extractDlg->show();
        });
        
        midRow->addWidget(m_btnArchive); midRow->addWidget(m_btnFolder);
        shamelaSettingsLayout->addLayout(midRow);

        m_btnLaunch = new RippleButton(); m_btnLaunch->setObjectName("btnLaunch"); m_btnLaunch->setFixedHeight(48);
        shamelaSettingsLayout->addWidget(m_btnLaunch);

        // -- زر إنشاء أيقونة سطح المكتب --
        m_btnCreateDesktopIcon = new RippleButton(); 
        m_btnCreateDesktopIcon->setObjectName("btnCreateDesktopIcon"); 
        m_btnCreateDesktopIcon->setFixedHeight(48);
        
        connect(m_btnCreateDesktopIcon, &QPushButton::clicked, this, [this]() {
            DesktopIconChoiceDialog choiceDlg(m_currentLang, this);
            choiceDlg.setStyleSheet(this->styleSheet()); 
            
            if (choiceDlg.exec() == QDialog::Accepted) {
                QString appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
                QDir dir(appsPath);
                if (!dir.exists()) dir.mkpath(".");
                
                if (choiceDlg.resultChoice == DesktopIconChoiceDialog::Shamela) {
                    if (m_shamelaPath.isEmpty() || !QFile::exists(m_shamelaPath)) {
                        QMessageBox::warning(this, m_currentLang == Language::Arabic ? "خطأ" : "Error", 
                                             m_currentLang == Language::Arabic ? "مسار الشاملة غير محدد أو غير موجود!" : "Shamela path is not set or not found!");
                        return;
                    }
                    QString desktopFilePath = dir.absoluteFilePath("shamela.desktop");
                    QFile dFile(desktopFilePath);
                    if (dFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&dFile);
                        out << "[Desktop Entry]\n"
                            << "Version=1.0\n"
                            << "Name=المكتبة الشاملة\n"
                            << "Name[en]=Shamela Library\n"
                            << "Comment=فتح المكتبة الشاملة\n"
                            << "Exec=\"" << m_shamelaPath << "\"\n"
                            << "Icon=accessories-dictionary\n"
                            << "Terminal=false\n"
                            << "Type=Application\n"
                            << "Categories=Education;Literature;\n";
                        dFile.close();
                        dFile.setPermissions(dFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther | QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
                        m_hasShamelaDesktop = true;
                        saveSettings();
                        QMessageBox::information(this, m_currentLang == Language::Arabic ? "نجاح" : "Success", 
                                                 m_currentLang == Language::Arabic ? "تم إنشاء أيقونة الشاملة بنجاح في مجلد التطبيقات." : "Shamela desktop icon created successfully.");
                    } else {
                        QMessageBox::critical(this, "خطأ", "فشل إنشاء ملف الأيقونة!");
                    }
                } else if (choiceDlg.resultChoice == DesktopIconChoiceDialog::App) {
                    QString appPath = QCoreApplication::applicationFilePath();
                    QString desktopFilePath = dir.absoluteFilePath("shamela-opener.desktop");
                    QFile dFile(desktopFilePath);
                    if (dFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&dFile);
                        out << "[Desktop Entry]\n"
                            << "Version=1.0\n"
                            << "Name=فاتح الشاملة\n"
                            << "Name[en]=Shamela Opener\n"
                            << "Comment=أداة لتثبيت وتشغيل المكتبة الشاملة\n"
                            << "Exec=\"" << appPath << "\"\n"
                            << "Icon=system-run\n"
                            << "Terminal=false\n"
                            << "Type=Application\n"
                            << "Categories=Utility;\n";
                        dFile.close();
                        dFile.setPermissions(dFile.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther | QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
                        m_hasAppDesktop = true;
                        saveSettings();
                        QMessageBox::information(this, m_currentLang == Language::Arabic ? "نجاح" : "Success", 
                                                 m_currentLang == Language::Arabic ? "تم إنشاء أيقونة هذا البرنامج بنجاح في مجلد التطبيقات." : "App desktop icon created successfully.");
                    } else {
                        QMessageBox::critical(this, "خطأ", "فشل إنشاء ملف الأيقونة!");
                    }
                }
            }
        });
        
        shamelaSettingsLayout->addWidget(m_btnCreateDesktopIcon);

        shamelaSettingsLayout->addStretch();
        
        // -- زر حذف ملفات سطح المكتب --
        m_btnDelete = new RippleButton(); m_btnDelete->setObjectName("btnDelete"); m_btnDelete->setFixedHeight(48);
        
        connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
            QString appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
            QDir dir(appsPath);
            
            bool deleted = false;
            
            QString shamelaDesktop = dir.absoluteFilePath("shamela.desktop");
            if (QFile::exists(shamelaDesktop)) {
                if (QFile::remove(shamelaDesktop)) {
                    m_hasShamelaDesktop = false;
                    deleted = true;
                }
            }
            
            QString appDesktop = dir.absoluteFilePath("shamela-opener.desktop");
            if (QFile::exists(appDesktop)) {
                if (QFile::remove(appDesktop)) {
                    m_hasAppDesktop = false;
                    deleted = true;
                }
            }
            
            if (deleted) {
                saveSettings();
                QMessageBox::information(this, 
                    m_currentLang == Language::Arabic ? "نجاح" : "Success", 
                    m_currentLang == Language::Arabic ? "تم حذف ملفات سطح المكتب التي أنشأها البرنامج بنجاح." : "Desktop files created by this app were deleted successfully.");
            } else {
                QMessageBox::information(this, 
                    m_currentLang == Language::Arabic ? "تنبيه" : "Notice", 
                    m_currentLang == Language::Arabic ? "لا توجد ملفات سطح مكتب ليتم حذفها." : "No desktop files found to delete.");
            }
        });
        
        shamelaSettingsLayout->addWidget(m_btnDelete);

        m_stackedWidget->addWidget(m_shamelaSettingsPage);

        m_settingsPage = new QWidget(this);
        auto* settingsLayout = new QVBoxLayout(m_settingsPage);
        settingsLayout->setContentsMargins(0, 0, 0, 0);
        settingsLayout->setSpacing(12);

        m_btnToggleLang = new RippleButton(); 
        m_btnToggleLang->setObjectName("btnSetting"); 
        m_btnToggleLang->setFixedHeight(48);

        m_btnToggleTheme = new RippleButton(); 
        m_btnToggleTheme->setObjectName("btnSetting"); 
        m_btnToggleTheme->setFixedHeight(48);

        connect(m_btnToggleLang, &QPushButton::clicked, this, [this]() {
            setLanguage(m_currentLang == Language::Arabic ? Language::English : Language::Arabic);
            saveSettings(); 
        });

        connect(m_btnToggleTheme, &QPushButton::clicked, this, [this]() {
            ThemeSelectionDialog themeDlg(m_currentLang, this);
            themeDlg.setStyleSheet(this->styleSheet());
            
            if (themeDlg.exec() == QDialog::Accepted) {
                int selection = themeDlg.selectedTheme;
                
                if (selection >= 0 && selection <= 4) { 
                    m_isCustomTheme = false;
                    applyTheme(static_cast<ThemeMode>(selection));
                } else if (selection == 5) {  
                    CustomThemeDialog customDlg(m_colors, this);
                    customDlg.setStyleSheet(this->styleSheet());
                    
                    if (customDlg.exec() == QDialog::Accepted) {
                        applyCustomTheme(customDlg.m_colors);
                    }
                }
                saveSettings(); 
            }
        });

        settingsLayout->addWidget(m_btnToggleLang);
        settingsLayout->addWidget(m_btnToggleTheme);
        settingsLayout->addStretch();

        m_stackedWidget->addWidget(m_settingsPage);

        root->addWidget(m_stackedWidget);
    }

    void buildSidebar() {
        m_sidebar = new QWidget(this);
        m_sidebar->setObjectName("sidebar");
        
        m_sidebar->setGeometry(-250, 0, 250, height()); 

        auto* layout = new QVBoxLayout(m_sidebar);
        layout->setContentsMargins(10, 60, 10, 20);
        layout->setSpacing(8);

        m_btnNavMain = new RippleButton();
        m_btnNavMain->setObjectName("btnSidebarItem");
        
        m_btnNavShamelaSettings = new RippleButton();
        m_btnNavShamelaSettings->setObjectName("btnSidebarItem");

        m_btnNavSettings = new RippleButton();
        m_btnNavSettings->setObjectName("btnSidebarItem");
        
        layout->addWidget(m_btnNavMain);
        layout->addWidget(m_btnNavShamelaSettings);
        layout->addWidget(m_btnNavSettings);
        layout->addStretch();

        connect(m_btnNavMain, &QPushButton::clicked, this, [this]{
            m_stackedWidget->setCurrentIndex(0);
            toggleSidebar();
        });
        connect(m_btnNavShamelaSettings, &QPushButton::clicked, this, [this]{
            m_stackedWidget->setCurrentIndex(1);
            toggleSidebar();
        });
        connect(m_btnNavSettings, &QPushButton::clicked, this, [this]{
            m_stackedWidget->setCurrentIndex(2);
            toggleSidebar();
        });
    }

    void toggleSidebar() {
        m_isSidebarOpen = !m_isSidebarOpen;
        
        if (m_isSidebarOpen) {
            m_sidebar->raise(); 
        }

        auto* anim = new QPropertyAnimation(m_sidebar, "geometry");
        anim->setDuration(300);
        anim->setEasingCurve(QEasingCurve::OutQuint);

        if (m_isSidebarOpen) {
            anim->setStartValue(QRect(-250, 0, 250, height()));
            anim->setEndValue(QRect(0, 0, 250, height()));
        } else {
            anim->setStartValue(QRect(0, 0, 250, height()));
            anim->setEndValue(QRect(-250, 0, 250, height()));
        }
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void saveSettings() {
        QString configPath = QDir::homePath() + "/.shamela_path.txt";
        QFile file(configPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            
            out << "Path=" << m_shamelaPath << "\n";
            out << "Language=" << (m_currentLang == Language::Arabic ? "Arabic" : "English") << "\n";
            out << "HasShamelaDesktop=" << (m_hasShamelaDesktop ? "1" : "0") << "\n";
            out << "HasAppDesktop=" << (m_hasAppDesktop ? "1" : "0") << "\n";
            
            if (m_isCustomTheme) {
                out << "Theme=Custom\n";
                out << "Base=" << m_colors.Base << "\n";
                out << "Surface=" << m_colors.Surface << "\n";
                out << "Text=" << m_colors.Text << "\n";
                out << "Hover=" << m_colors.Hover << "\n";
                out << "Accent1=" << m_colors.Accent1 << "\n";
                out << "Accent2=" << m_colors.Accent2 << "\n";
                out << "Accent3=" << m_colors.Accent3 << "\n";
                out << "Accent4=" << m_colors.Accent4 << "\n";
                out << "Danger=" << m_colors.Danger << "\n";
                out << "IsDark=" << (m_colors.isDark ? "1" : "0") << "\n";
            } else {
                QString themeName;
                switch(m_theme) {
                    case ThemeMode::Latte: themeName = "Latte"; break;
                    case ThemeMode::ShamelaClassic: themeName = "ShamelaClassic"; break;
                    case ThemeMode::Nord: themeName = "Nord"; break;
                    case ThemeMode::Gruvbox: themeName = "Gruvbox"; break;
                    case ThemeMode::Mocha:
                    default: themeName = "Mocha"; break; 
                }
                out << "Theme=" << themeName << "\n";
            }
            file.close();
        }
    }

    void loadSettings() {
        QString configPath = QDir::homePath() + "/.shamela_path.txt";
        QFile file(configPath);
        
        m_currentLang = Language::Arabic;
        m_theme = ThemeMode::Mocha;
        m_isCustomTheme = false;
        m_shamelaPath = "";
        m_hasShamelaDesktop = false;
        m_hasAppDesktop = false;

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty()) continue;
                
                if (!line.contains("=")) {
                    if (m_shamelaPath.isEmpty() && line.contains("/")) {
                        m_shamelaPath = line;
                    }
                    continue;
                }

                QStringList parts = line.split("=");
                if (parts.size() < 2) continue;
                
                QString key = parts[0].trimmed();
                QString val = parts[1].trimmed();

                if (key == "Path") m_shamelaPath = val;
                else if (key == "Language") {
                    if (val == "English") m_currentLang = Language::English;
                    else m_currentLang = Language::Arabic;
                }
                else if (key == "HasShamelaDesktop") m_hasShamelaDesktop = (val == "1");
                else if (key == "HasAppDesktop") m_hasAppDesktop = (val == "1");
                else if (key == "Theme") {
                    if (val == "Custom") m_isCustomTheme = true;
                    else if (val == "Latte") m_theme = ThemeMode::Latte;
                    else if (val == "ShamelaClassic") m_theme = ThemeMode::ShamelaClassic;
                    else if (val == "Nord") m_theme = ThemeMode::Nord;
                    else if (val == "Gruvbox") m_theme = ThemeMode::Gruvbox;
                    else m_theme = ThemeMode::Mocha;
                }
                else if (m_isCustomTheme) {
                    if (key == "Base") m_colors.Base = val;
                    else if (key == "Surface") m_colors.Surface = val;
                    else if (key == "Text") m_colors.Text = val;
                    else if (key == "Hover") m_colors.Hover = val;
                    else if (key == "Accent1") m_colors.Accent1 = val;
                    else if (key == "Accent2") m_colors.Accent2 = val;
                    else if (key == "Accent3") m_colors.Accent3 = val;
                    else if (key == "Accent4") m_colors.Accent4 = val;
                    else if (key == "Danger") m_colors.Danger = val;
                    else if (key == "IsDark") m_colors.isDark = (val == "1");
                }
            }
            file.close();
        }
    }

    void applyTheme(ThemeMode mode) {
        m_theme = mode;
        m_isCustomTheme = false;
        
        switch(m_theme) {
            case ThemeMode::Latte:          m_colors = LatteTheme; break;
            case ThemeMode::ShamelaClassic: m_colors = ShamelaClassicTheme; break;
            case ThemeMode::Nord:           m_colors = NordTheme; break;
            case ThemeMode::Gruvbox:        m_colors = GruvboxTheme; break;
            case ThemeMode::Mocha:
            default:                        m_colors = MochaTheme; break; 
        }

        applyCurrentColors();
    }

    void applyCustomTheme(const ThemeColors& customColors) {
        m_colors = customColors;
        m_isCustomTheme = true;
        applyCurrentColors();
    }

    void applyCurrentColors() {
        updateDynamicTexts();

        QString qss = QStringLiteral(R"(
            QWidget { color: %1; }
            QDialog { background-color: %2; }
            QPushButton { font-size: 14px; font-weight: bold; border: none; border-radius: 8px; padding: 12px; }
            
            QProgressBar { border: 1px solid %3; border-radius: 4px; text-align: center; color: %1; background: %2; }
            QProgressBar::chunk { background-color: %7; border-radius: 4px; }
            
            QPushButton#hamburgerBtn { background: transparent; color: %1; padding: 4px; border-radius: 4px; }
            QPushButton#hamburgerBtn:hover { background: %4; }

            QWidget#sidebar { background-color: %3; border-right: 1px solid %4; }
            QPushButton#btnSidebarItem { background: transparent; color: %1; text-align: left; padding: 12px 20px; }
            QPushButton#btnSidebarItem:hover { background: %4; }
            
            QPushButton#btnDownload, QPushButton#btnCheckDeps { background: %3; color: %5; }
            QPushButton#btnDownload:hover, QPushButton#btnCheckDeps:hover { background: %5; color: %2; }

            QPushButton#btnArchive { background: %3; color: %5; }
            QPushButton#btnArchive:hover { background: %5; color: %2; }
            
            QPushButton#btnFolder, QPushButton#btnCreateDesktopIcon { background: %3; color: %6; }
            QPushButton#btnFolder:hover, QPushButton#btnCreateDesktopIcon:hover { background: %6; color: %2; }

            QPushButton#btnOpen { background: %3; color: %7; }
            QPushButton#btnOpen:hover { background: %7; color: %2; }

            QPushButton#btnLaunch { background: %3; color: %8; }
            QPushButton#btnLaunch:hover { background: %8; color: %2; }

            QPushButton#btnDelete { background: transparent; border: 1px solid %9; color: %9; }
            QPushButton#btnDelete:hover { background: %9; color: %2; }

            QPushButton#btnSetting { background: %3; color: %1; }
            QPushButton#btnSetting:hover { background: %4; }
        )")
        .arg(m_colors.Text, m_colors.Base, m_colors.Surface, m_colors.Hover)
        .arg(m_colors.Accent1, m_colors.Accent2, m_colors.Accent3, m_colors.Accent4, m_colors.Danger);

        this->setStyleSheet(qss);
        update();
    }

    void setLanguage(Language lang) {
        m_currentLang = lang;
        bool isAr = (lang == Language::Arabic);
        
        m_mainContent->setLayoutDirection(isAr ? Qt::RightToLeft : Qt::LeftToRight);
        m_sidebar->setLayoutDirection(isAr ? Qt::RightToLeft : Qt::LeftToRight);

        setWindowTitle(isAr ? "فاتح الشاملة" : "Shamela Opener");

        m_btnNavMain->setText(isAr ? "🏠  القائمة الرئيسية" : "🏠  Main Menu");
        m_btnNavShamelaSettings->setText(isAr ? "📚  إعداد الشاملة" : "📚  Shamela Settings");
        m_btnNavSettings->setText(isAr ? "⚙️  الإعدادات" : "⚙️  Settings");
        
        m_btnDownload->setText(isAr ? "تحميل الشاملة لو لم تملكها" : "Download Shamela if you don't have it");
        m_btnCheckDeps->setText(isAr ? "فحص الاعتماديات" : "Check Dependencies");
        m_btnArchive->setText(isAr ? "تثبيت من ملف مضغوط" : "Install from archive");
        
        m_btnFolder->setText(isAr ? "تحديد مسار الملف" : "Set File Path");
        m_btnOpen->setText(isAr ? "فتح الشاملة" : "Open Shamela");
        m_btnCreateDesktopIcon->setText(isAr ? "اصنع أيقونة سطح المكتب" : "Create Desktop Icon");
        
        QString arLaunchText = QString("%1ضع ملف %2launch.sh%3 لفتح الشاملة%3")
                               .arg(QString(QChar(0x202B)))
                               .arg(QString(QChar(0x202A)))
                               .arg(QString(QChar(0x202C)));

        m_btnLaunch->setText(isAr ? arLaunchText : "Put launch.sh file to open Shamela");
        m_btnDelete->setText(isAr ? "حذف ملفات سطح المكتب الخاصة بالبرنامج" : "Delete desktop files for this app");

        updateDynamicTexts();
    }

    void updateDynamicTexts() {
        bool isAr = (m_currentLang == Language::Arabic);
        
        QString themeName;
        if (m_isCustomTheme) {
            themeName = isAr ? "🎨 مخصص" : "🎨 Custom";
        } else {
            switch(m_theme) {
                case ThemeMode::Latte:          themeName = "☀️ Latte"; break;
                case ThemeMode::ShamelaClassic: themeName = "📜 Shamela Classic"; break;
                case ThemeMode::Nord:           themeName = "❄️ Nord"; break;
                case ThemeMode::Gruvbox:        themeName = "📦 Gruvbox"; break;
                case ThemeMode::Mocha:
                default:                        themeName = "☕ Mocha"; break;
            }
        }

        m_btnToggleTheme->setText(isAr ? "المظهر: " + themeName : "Theme: " + themeName);

        QString langText = isAr ? "اللغة: العربية 🇸🇦" : "Language: English 🇬🇧";
        m_btnToggleLang->setText(langText);
    }

    QPointF m_currentGlowPos, m_targetGlowPos;
    QTimer* m_glowTimer{};
    
    QString m_shamelaPath;
    ThemeMode m_theme; 
    ThemeColors m_colors;
    bool m_isCustomTheme = false;
    Language m_currentLang;
    bool m_hasShamelaDesktop = false;
    bool m_hasAppDesktop = false;
    
    bool m_isSidebarOpen = false;
    QWidget *m_sidebar{};
    QWidget *m_mainContent{};
    QWidget *m_topBar{};
    QStackedWidget *m_stackedWidget{};
    QWidget *m_mainMenuPage{}, *m_shamelaSettingsPage{}, *m_settingsPage{};
    QPushButton *m_hamburgerBtn{};
    
    RippleButton *m_btnNavMain{}, *m_btnNavShamelaSettings{}, *m_btnNavSettings{};
    RippleButton *m_btnDownload{}, *m_btnCheckDeps{}, *m_btnArchive{}, *m_btnFolder{}, *m_btnOpen{}, *m_btnLaunch{}, *m_btnCreateDesktopIcon{}, *m_btnDelete{};
    RippleButton *m_btnToggleLang{}, *m_btnToggleTheme{};
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(11);
    font.setFamily("Segoe UI, Tahoma, Noto Sans Arabic, sans-serif");
    app.setFont(font);

    GlowWindow window;
    window.show();
    return app.exec();
}
#include "main.moc"
