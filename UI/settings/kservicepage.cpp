#include "kservicepage.h"
#include "Common/notifier.h"
#include "MediaLibrary/animeprovider.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include <QVBoxLayout>
#include "UI/inputdialog.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Playlist/playlist.h"
#include <QSettings>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#ifdef KSERVICE
#include "Service/klogin.h"
#include "Service/kregister.h"
#include "Service/kservice.h"
#endif


KServicePage::KServicePage(QWidget *parent) : SettingPage(parent)
{
    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
#ifdef KSERVICE
    itemVLayout->addWidget(initAccountArea());
#endif
    itemVLayout->addWidget(initEpMatchArea());
#ifdef KSERVICE
    itemVLayout->addWidget(initInfoRetrievalArea());
#endif
    itemVLayout->addStretch(1);
}

#ifdef KSERVICE
SettingItemArea *KServicePage::initAccountArea()
{
    SettingItemArea *accountArea = new SettingItemArea(tr("KikoPlay Account"), this);
    QWidget *accountWidget = new QWidget(accountArea);
    QLabel *userLabel = new QLabel(accountWidget);
    QLabel *emailLabel = new QLabel(accountWidget);
    QFont accountFont(GlobalObjects::normalFont);
    accountFont.setPointSizeF(GlobalObjects::fontSize(12));
    for (QLabel *label : {userLabel, emailLabel})
    {
        label->setFont(accountFont);
        label->setTextFormat(Qt::PlainText);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setWordWrap(true);
    }

    KPushButton *loginBtn = new KPushButton(tr("Login"), accountWidget);
    KPushButton *registerBtn = new KPushButton(tr("Register"), accountWidget);
    KPushButton *logoutBtn = new KPushButton(tr("Log Out"), accountWidget);
    for (KPushButton *button : {loginBtn, registerBtn, logoutBtn})
    {
        button->setAutoDefault(false);
    }

    QVBoxLayout *infoLayout = new QVBoxLayout;
    infoLayout->setSpacing(4);
    infoLayout->addWidget(userLabel);
    infoLayout->addWidget(emailLabel);
    QHBoxLayout *accountLayout = new QHBoxLayout(accountWidget);
    accountLayout->setContentsMargins(0, 0, 0, 0);
    accountLayout->setSpacing(8);
    accountLayout->addLayout(infoLayout, 1);
    accountLayout->addWidget(loginBtn);
    accountLayout->addWidget(registerBtn);
    accountLayout->addWidget(logoutBtn);
    accountArea->addItem(accountWidget);

    KService *service = KService::instance();
    const auto updateAccount = [=](){
        const KServiceAccount account = service->account();
        userLabel->setText(account.loggedIn ? tr("User Name: %1").arg(account.userName) : tr("Not logged in"));
        emailLabel->setText(account.loggedIn ? tr("Email: %1").arg(account.email) : QString());
        emailLabel->setVisible(account.loggedIn && !account.email.isEmpty());
        loginBtn->setVisible(!account.loggedIn);
        registerBtn->setVisible(!account.loggedIn);
        logoutBtn->setVisible(account.loggedIn);
        logoutBtn->setEnabled(true);
    };
    QObject::connect(service, &KService::accountChanged, this, updateAccount);
    updateAccount();

    QObject::connect(loginBtn, &QPushButton::clicked, this, [=](){
        KLogin login(this);
        login.exec();
    });
    QObject::connect(registerBtn, &QPushButton::clicked, this, [=](){
        KRegister registration(this);
        if (registration.exec() == QDialog::Accepted)
        {
            KLogin login(this, registration.registeredEmail);
            login.exec();
        }
    });
    QObject::connect(logoutBtn, &QPushButton::clicked, this, [=](){
        logoutBtn->setEnabled(false);
        service->logout();
    });

    return accountArea;
}
#endif

SettingItemArea *KServicePage::initEpMatchArea()
{
    SettingItemArea *epMatchArea = new SettingItemArea(tr("Episode Matching"), this);

    ElaToggleSwitch *autoMatchSwitch = new ElaToggleSwitch(this);
    autoMatchSwitch->setIsToggled(GlobalObjects::playlist->isAutoMatch());
    epMatchArea->addItem(tr("Auto Epsidoe Matching"), autoMatchSwitch);

    ElaComboBox *defaultMatchCombo = new ElaComboBox(this);
    const auto matchProviders = GlobalObjects::animeProvider->getMatchProviders();
    const QString defaultScriptId = GlobalObjects::animeProvider->defaultMatchScript();
#ifdef KSERVICE
    defaultMatchCombo->addItem(tr("KikoPlay Service"), QString());
#endif
    for (const auto &p : matchProviders)
    {
        defaultMatchCombo->addItem(p.first, p.second);
    }
    int defaultIndex = defaultScriptId.isEmpty() ? -1 : defaultMatchCombo->findData(defaultScriptId);
#ifdef KSERVICE
    if (KService::instance()->enableKServiceMatch()) defaultIndex = 0;
#endif
    defaultMatchCombo->setCurrentIndex(defaultIndex);
    epMatchArea->addItem(tr("Default Match Method"), defaultMatchCombo);

    KPushButton *matchFilterBtn = new KPushButton(tr("Edit"), this);
    epMatchArea->addItem(tr("Match Filter Setting"), matchFilterBtn);

#ifdef KSERVICE
    ElaToggleSwitch *kserviceAutoAddSrcSwitch = new ElaToggleSwitch(this);
    kserviceAutoAddSrcSwitch->setIsToggled(KService::instance()->enableKServiceAutoAddDanmuSrc());
    epMatchArea->addItem(tr("Recognition succeeded: auto-add danmu source (if any)"), kserviceAutoAddSrcSwitch);

    QObject::connect(kserviceAutoAddSrcSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        KService::instance()->setEnableKServiceAutoAddDanmuSrc(checked);
    });
#endif

    QObject::connect(autoMatchSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::playlist->setAutoMatch(checked);
    });

    QObject::connect(defaultMatchCombo, (void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [=](int index){
        if (index < 0) return;
        const QString scriptId = defaultMatchCombo->itemData(index).toString();
#ifdef KSERVICE
        if (scriptId.isEmpty())
        {
            KService::instance()->setEnableKServiceMatch(true);
            return;
        }
#endif
        if (!scriptId.isEmpty())
        {
            GlobalObjects::animeProvider->setDefaultMatchScript(scriptId);
#ifdef KSERVICE
            KService::instance()->setEnableKServiceMatch(false);
#endif
        }
    });

    QObject::connect(matchFilterBtn, &QPushButton::clicked, this, [=](){
        InputDialog inputDialog(tr("Macth Filter"),
                                tr("Set rules line by line, supporting regular expressions.\nKikoPlay will skip items matched by the rules during the match process."),
                                GlobalObjects::playlist->matchFilters(),
                                true,this, "DialogSize/MatchFilter");
        if (QDialog::Accepted == inputDialog.exec())
        {
            GlobalObjects::playlist->setMatchFilters(inputDialog.text.trimmed());
        }
    });

    return epMatchArea;
}


#ifdef KSERVICE
SettingItemArea *KServicePage::initInfoRetrievalArea()
{
    SettingItemArea *infoRetrievalArea = new SettingItemArea(tr("Information Retrieval"), this);

    KPushButton *kserviceLibraryOrderBtn = new KPushButton(tr("Edit"), this);
    infoRetrievalArea->addItem(tr("KService Library Download Priority"), kserviceLibraryOrderBtn);
    QObject::connect(kserviceLibraryOrderBtn, &QPushButton::clicked, this, [=](){
        KLibraryOrderDialog dialog(this);
        dialog.exec();
    });

    ElaToggleSwitch *kserviceAnimeProfileSwitch = new ElaToggleSwitch(this);
    infoRetrievalArea->addItem(tr("Use KService Anime Profiles"), kserviceAnimeProfileSwitch);
    QObject::connect(kserviceAnimeProfileSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        KService::instance()->setEnableKServiceAnimeProfile(checked);
    });
    QObject::connect(KService::instance(), &KService::animeProfileSettingChanged, kserviceAnimeProfileSwitch, [=](bool){
        const QSignalBlocker blocker(kserviceAnimeProfileSwitch);
        kserviceAnimeProfileSwitch->setIsToggled(KService::instance()->enableKServiceAnimeProfile());
    });
    {
        const QSignalBlocker blocker(kserviceAnimeProfileSwitch);
        kserviceAnimeProfileSwitch->setIsToggled(KService::instance()->enableKServiceAnimeProfile());
    }

    return infoRetrievalArea;
}

KLibraryOrderDialog::KLibraryOrderDialog(QWidget *parent) : CFramelessDialog(tr("Library Download Priority"), parent, true)
{
    QLabel *tipLabel = new QLabel(tr("When multiple data sources are obtained from KService, set the download priority:\n(Drag to change the order)"), this);
    sourecOrderView = new QListWidget(this);
    sourecOrderView->setObjectName(QStringLiteral("SourceOrderView"));
    QFont f(GlobalObjects::normalFont);
    f.setPointSizeF(GlobalObjects::fontSize(12));
    sourecOrderView->setFont(f);
    sourecOrderView->setDragEnabled(true);
    sourecOrderView->setAcceptDrops(true);
    sourecOrderView->setDragDropMode(QAbstractItemView::InternalMove);
    sourecOrderView->setDropIndicatorShown(true);
    sourecOrderView->setDefaultDropAction(Qt::MoveAction);
    sourecOrderView->setSelectionMode(QListWidget::SingleSelection);

    auto librarySrc = KService::instance()->getLibrarySource();
    for (auto &src : librarySrc)
    {
        QListWidgetItem *item = new QListWidgetItem(src.first, sourecOrderView, src.second.first);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(src.second.second ? Qt::Checked : Qt::Unchecked);
    }

    QGridLayout *kGLayout = new QGridLayout(this);
    kGLayout->addWidget(tipLabel, 0, 0);
    kGLayout->addWidget(sourecOrderView, 1, 0);
    kGLayout->setRowStretch(1, 1);

    setSizeSettingKey("DialogSize/KLibraryOrderDialog",QSize(100, 120));
}

void KLibraryOrderDialog::onAccept()
{
    QList<QPair<int, bool>> indexSelected;
    bool hasChecked = false;
    for (int i = 0; i < sourecOrderView->count(); ++i)
    {
        int src = sourecOrderView->item(i)->type();
        bool checked = sourecOrderView->item(i)->checkState() == Qt::Checked;
        indexSelected.append({src, checked});
        if (checked) hasChecked = true;
    }
    if (!hasChecked)
    {
        showMessage(tr("At least one source must be selected"), NM_ERROR | NM_HIDE);
        return;
    }
    KService::instance()->setLibrarySourceIndex(indexSelected);
    CFramelessDialog::onAccept();
}
#endif
