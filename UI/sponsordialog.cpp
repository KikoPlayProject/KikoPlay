#include "sponsordialog.h"
#include "globalobjects.h"
#include "Common/notifier.h"
#include "ela/ElaIcon.h"
#include "widgets/kpushbutton.h"
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QUrl>
#include <QVBoxLayout>

SponsorDialog::SponsorDialog(QWidget *parent)
    : CFramelessDialog(tr("Support KikoPlay"), parent)
{
    setResizeable(false);

    auto makeLabel = [](const QString &text, qreal size, QWidget *parent, bool secondary = false) {
        QLabel *label = new QLabel(text, parent);
        QFont font(GlobalObjects::normalFont);
        font.setPixelSize(qRound(GlobalObjects::fontSize(size)));
        label->setFont(font);
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        if (secondary) label->setObjectName(QStringLiteral("SponsorSecondaryText"));
        return label;
    };

    auto addCardHeading = [&](QVBoxLayout *layout, const QString &text,
                              ElaIconType::IconName icon, const QString &iconName) {
        QWidget *card = layout->parentWidget();
        QLabel *iconLabel = new QLabel(QString(QChar(static_cast<ushort>(icon))), card);
        QFont iconFont(QStringLiteral("ElaAwesome"));
        iconFont.setPixelSize(qRound(GlobalObjects::fontSize(18)));
        iconLabel->setFont(iconFont);
        iconLabel->setObjectName(iconName);
        iconLabel->setAlignment(Qt::AlignCenter);
        QHBoxLayout *headingLayout = new QHBoxLayout;
        headingLayout->setSpacing(8);
        headingLayout->addWidget(iconLabel);
        headingLayout->addWidget(makeLabel(text, 15, card), 1);
        layout->addLayout(headingLayout);
    };

    QLabel *heading = makeLabel(tr("Thank you for your support"), 22, this);
    QLabel *description = makeLabel(
        tr("If KikoPlay helps you, consider supporting its development and maintenance."),
        13, this, true);

    QFrame *afdianCard = new QFrame(this);
    afdianCard->setObjectName(QStringLiteral("SponsorCard"));
    QVBoxLayout *afdianLayout = new QVBoxLayout(afdianCard);
    afdianLayout->setContentsMargins(18, 17, 18, 15);
    afdianLayout->setSpacing(10);
    addCardHeading(afdianLayout, tr("Afdian"), ElaIconType::Bolt,
                   QStringLiteral("SponsorAfdianIcon"));
    afdianLayout->addStretch(1);
    afdianLayout->addWidget(makeLabel(tr("Power KikoPlay"), 24, afdianCard));
    afdianLayout->addWidget(makeLabel(
        tr("Visit KikoPlay on Afdian and choose how you would like to support the project."),
        13, afdianCard, true));
    afdianLayout->addStretch(1);

    KPushButton *afdianButton = new KPushButton(tr("Open Afdian"), afdianCard);
    afdianButton->setObjectName(QStringLiteral("SponsorAfdianButton"));
    afdianButton->setAutoDefault(false);
    afdianButton->setMinimumHeight(36);
    afdianButton->setIcon(ElaIcon::getInstance()->getElaIcon(
        ElaIconType::ArrowUpRightFromSquare, 16, QColor(220, 220, 220)));
    afdianButton->setLayoutDirection(Qt::RightToLeft);
    afdianButton->setToolTip(QStringLiteral("https://afdian.com/a/KikoPlay"));
    afdianLayout->addWidget(afdianButton);
    QLabel *browserHint = makeLabel(tr("Opens in your browser"), 12, afdianCard, true);
    browserHint->setAlignment(Qt::AlignCenter);
    afdianLayout->addWidget(browserHint);

    QFrame *wechatCard = new QFrame(this);
    wechatCard->setObjectName(QStringLiteral("SponsorCard"));
    QVBoxLayout *wechatLayout = new QVBoxLayout(wechatCard);
    wechatLayout->setContentsMargins(18, 17, 18, 15);
    wechatLayout->setSpacing(10);
    addCardHeading(wechatLayout, tr("WeChat"), ElaIconType::Qrcode,
                   QStringLiteral("SponsorWechatIcon"));
    wechatLayout->addSpacing(6);

    QLabel *qrCode = new QLabel(wechatCard);
    qrCode->setObjectName(QStringLiteral("SponsorQRCode"));
    qrCode->setAccessibleName(tr("WeChat sponsorship QR code"));
    qrCode->setAlignment(Qt::AlignCenter);
    qrCode->setFixedSize(192, 192);
    const qreal pixelRatio = devicePixelRatioF();
    const int imageSize = qRound(172 * pixelRatio);
    QPixmap qrPixmap(QStringLiteral(":/res/images/support.png"));
    qrPixmap = qrPixmap.scaled(imageSize, imageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    qrPixmap.setDevicePixelRatio(pixelRatio);
    qrCode->setPixmap(qrPixmap);
    wechatLayout->addWidget(qrCode, 0, Qt::AlignHCenter);
    wechatLayout->addStretch(1);
    QLabel *scanHint = makeLabel(tr("Scan with WeChat"), 12, wechatCard, true);
    scanHint->setAlignment(Qt::AlignCenter);
    wechatLayout->addWidget(scanHint);

    QHBoxLayout *methodsLayout = new QHBoxLayout;
    methodsLayout->setSpacing(16);
    methodsLayout->addWidget(afdianCard, 1);
    methodsLayout->addWidget(wechatCard, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 20);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(heading);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(description);
    mainLayout->addSpacing(23);
    mainLayout->addLayout(methodsLayout, 1);

    QObject::connect(afdianButton, &QPushButton::clicked, this, [this]() {
        if (!QDesktopServices::openUrl(QUrl(QStringLiteral("https://afdian.com/a/KikoPlay"))))
        {
            showMessage(tr("Unable to open the browser."), NM_ERROR | NM_HIDE);
        }
    });

    resize(QSize(620, 450).expandedTo(sizeHint()));
}
