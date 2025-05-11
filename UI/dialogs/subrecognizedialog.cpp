#include "subrecognizedialog.h"
#include <QLabel>
#include <QGridLayout>
#include <QStackedWidget>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QTableView>
#include <QFileDialog>
#include <QTreeView>
#include <QListView>
#include <QHeaderView>
#include "Common/notifier.h"
#include "Play/Subtitle/subtitleloader.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaSlider.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/inputdialog.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/kpushbutton.h"
#include "Play/Subtitle/subtitlerecognizer.h"
#include "Play/Subtitle/subtitleeditmodel.h"
#include "Play/Subtitle/subtitletranslator.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"


#define SETTING_KEY_SUB_RECOGNIZE_MODEL       "SubRecogize/Model"
#define SETTING_KEY_SUB_RECOGNIZE_LANG        "SubRecogize/Lang"
#define SETTING_KEY_SUB_RECOGNIZE_USE_CUDA    "SubRecogize/UseCuda"
#define SETTING_KEY_SUB_RECOGNIZE_USE_VAD     "SubRecogize/UseVAD"
#define SETTING_KEY_SUB_RECOGNIZE_VAD_THRES   "SubRecogize/VADThres"
#define SETTING_KEY_SUB_TRANSLATOR_DEFAULT    "SubRecogize/DefaultTranslator"

SubRecognizeDialog::SubRecognizeDialog(const QString &videoFile, QWidget *parent) : CFramelessDialog(tr("Subtitle Recognition"), parent)
{
    QLabel *modelTip = new QLabel(QString("%1&nbsp;<a style='color: rgb(96, 208, 252);' href = \"\">%2</a>").arg(tr("Model"), tr("More")), this);
    modelTip->setOpenExternalLinks(true);
    ElaComboBox *modelCombo = new ElaComboBox(this);

    QLabel *langTip = new QLabel(tr("Language"), this);
    ElaComboBox *langCombo = new ElaComboBox(this);

    ElaCheckBox *cudaCheck = new ElaCheckBox(tr("Enable CUDA-accelerated recognition"), this);

    QWidget *vadContainer = new QWidget(this);
    ElaCheckBox *vadCheck = new ElaCheckBox(tr("Enable VAD(Voice Activity Detection)"), this);
    QLabel *vadThresTip = new QLabel(tr("Thres"), this);
    ElaSlider *vadThresSlider = new ElaSlider(Qt::Orientation::Horizontal, this);
    vadThresSlider->setRange(1, 99);

    QHBoxLayout *vadHLayout = new QHBoxLayout(vadContainer);
    vadHLayout->setContentsMargins(0, 0, 0, 0);
    vadHLayout->addWidget(vadCheck);
    vadHLayout->addWidget(vadThresTip);
    vadHLayout->addWidget(vadThresSlider);
    vadHLayout->addStretch(1);

    QLabel *progressTip = new QLabel(this);
    QPushButton *startBtn = new KPushButton(tr("Start Recognition"), this);

    QGridLayout *subGLayout = new QGridLayout(this);
    subGLayout->addWidget(modelTip, 0, 0);
    subGLayout->addWidget(modelCombo, 1, 0);
    subGLayout->addWidget(langTip, 0, 1);
    subGLayout->addWidget(langCombo, 1, 1);
    subGLayout->addWidget(cudaCheck, 2, 0, 1, 2);
    subGLayout->addWidget(vadContainer, 3, 0, 1, 2);
    subGLayout->addWidget(progressTip, 4, 0);
    subGLayout->addWidget(startBtn, 4, 1, Qt::AlignVCenter | Qt::AlignRight);

    subGLayout->setColumnStretch(0, 1);
    subGLayout->setColumnStretch(1, 1);
    subGLayout->setRowStretch(5, 1);
    setSizeSettingKey("DialogSize/SubRecognizeDialog", QSize(520, 200));

    SubRecognizeOptions options;
    QDir folder(options.modelPathBase);
    const QFileInfoList infoList = folder.entryInfoList();
    int lastModelIndex = -1;
    const QString lastModelName = GlobalObjects::appSetting->value(SETTING_KEY_SUB_RECOGNIZE_MODEL, "").toString();
    for (const QFileInfo& fileInfo : infoList)
    {
        if (fileInfo.isFile() && fileInfo.suffix() == "bin")
        {
            modelCombo->addItem(fileInfo.baseName(), fileInfo.absoluteFilePath());
            if (fileInfo.baseName() == lastModelName && !lastModelName.isEmpty())
            {
                lastModelIndex = modelCombo->count() - 1;
            }
        }
    }
    if (lastModelIndex >= 0) modelCombo->setCurrentIndex(lastModelIndex);

    const QList<QPair<QString, QString>> languageList = {
        {tr("Auto Detect"), "auto"},
        {tr("English"), "en"},
        {tr("Japanese"), "ja"},
        {tr("Korean"), "ko"},
        {tr("Russian"), "ru"},
        {tr("French"), "fr"},
        {tr("German"), "de"},
        {tr("Chinese"), "zh"},
    };
    int lastLangIndex = -1;
    const QString lastLang = GlobalObjects::appSetting->value(SETTING_KEY_SUB_RECOGNIZE_LANG, "auto").toString();
    for (int i = 0; i < languageList.size(); ++i)
    {
        if (languageList[i].second == lastLang)
        {
            lastLangIndex = i;
        }
        langCombo->addItem(languageList[i].first, languageList[i].second);
    }
    if (lastLangIndex >= 0) langCombo->setCurrentIndex(lastLangIndex);

    cudaCheck->setChecked(GlobalObjects::appSetting->value(SETTING_KEY_SUB_RECOGNIZE_USE_CUDA, false).toBool());
    vadCheck->setChecked(GlobalObjects::appSetting->value(SETTING_KEY_SUB_RECOGNIZE_USE_VAD, true).toBool());
    vadThresTip->setVisible(vadCheck->isChecked());
    vadThresSlider->setVisible(vadCheck->isChecked());
    vadThresSlider->setValue(GlobalObjects::appSetting->value(SETTING_KEY_SUB_RECOGNIZE_VAD_THRES, 50).toInt());
    vadThresSlider->setMinimumWidth(120);
    vadThresTip->setText(tr("Thres(%1)").arg(vadThresSlider->value() / 100.0, 2, 'f', 2, QChar('0')));

    QObject::connect(vadCheck, &QCheckBox::clicked, this, [=](bool checked){
        vadThresTip->setVisible(checked);
        vadThresSlider->setVisible(checked);
    });

    QObject::connect(vadThresSlider, &ElaSlider::valueChanged, this, [=](int value){
         vadThresTip->setText(tr("Thres(%1)").arg(value / 100.0, 2, 'f', 2, QChar('0')));
    });

    QObject::connect(startBtn, &QPushButton::clicked, this, [=](){
        if (task)
        {
            task->cancel();
            startBtn->setEnabled(false);
            return;
        }
        const QString model = modelCombo->currentData().toString();
        if (model.isEmpty() || !QFile::exists(model))
        {
            showMessage(tr("Model File Invalid"), NM_HIDE | NM_ERROR);
            return;
        }
        SubRecognizeOptions options;
        options.modelPath = model;
        options.language = langCombo->currentData().toString();
        options.videoPath = videoFile;
        options.vadMode = vadCheck->isChecked() ? 1 : 0;
        options.whisperUseCuda = cudaCheck->isChecked();
        options.vadThres = vadThresSlider->value() / 100.0;

        modelCombo->setEnabled(false);
        langCombo->setEnabled(false);
        vadCheck->setEnabled(false);
        cudaCheck->setEnabled(false);
        vadThresSlider->setEnabled(false);
        startBtn->setText(tr("Stop Recognition"));

        GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_RECOGNIZE_MODEL, QFileInfo(model).baseName());
        GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_RECOGNIZE_LANG, options.language);
        GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_RECOGNIZE_USE_CUDA, options.whisperUseCuda);
        GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_RECOGNIZE_USE_VAD, options.vadMode != 0);
        GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_RECOGNIZE_VAD_THRES, vadThresSlider->value());

        task = new SubtitleRecognizer(options);
        QObject::connect(task, &SubtitleRecognizer::infoChanged, this, [=](const QString &content, int flag){
            progressTip->setText(content);
            if (flag & NM_ERROR)
            {
                this->showMessage(content, flag);
            }
        }, Qt::QueuedConnection);
        QObject::connect(task, &SubtitleRecognizer::progreeUpdated, this, [=](int progress){
            progressTip->setText(tr("Run Recognition...%1%").arg(progress));
        }, Qt::QueuedConnection);
        QObject::connect(task, &SubtitleRecognizer::recognizedDown, this, [=](const QString &subPath){
            SubRecognizeEditDialog subEdit(subPath, videoFile, false, this);
            if (subEdit.exec() == QDialog::Accepted)
            {
                CFramelessDialog::onAccept();
            }
        }, Qt::QueuedConnection);
        QObject::connect(task, &SubtitleRecognizer::statusChanged, this, [=](TaskStatus status){
            if (status == TaskStatus::Cancelled || status == TaskStatus::Failed || status == TaskStatus::Finished)
            {
                modelCombo->setEnabled(true);
                langCombo->setEnabled(true);
                vadCheck->setEnabled(true);
                cudaCheck->setEnabled(true);
                vadThresSlider->setEnabled(true);
                progressTip->clear();

                startBtn->setText(tr("Start Recognition"));
                startBtn->setEnabled(true);
                this->task = nullptr;
                this->showBusyState(false);
            }
        }, Qt::QueuedConnection);
        TaskPool::instance()->submitTask(task);
        showBusyState(true);

    });
}

SubRecognizeEditDialog::SubRecognizeEditDialog(const SubFile &_subFile, const QString &videoFilePath, bool translate, QWidget *parent) :
    CFramelessDialog(translate ? tr("Subtitle Translation") : tr("Subtitle Recognition"), parent)
{
    init(_subFile, videoFilePath, translate);
}

SubRecognizeEditDialog::SubRecognizeEditDialog(const QString &subFilePath, const QString &videoFilePath, bool translate, QWidget *parent) :
    CFramelessDialog(translate ? tr("Subtitle Translation") : tr("Subtitle Recognition"), parent)
{
    SubtitleLoader loader;
    init(loader.loadSubFile(subFilePath), videoFilePath, translate);
    QFile::remove(subFilePath);
}

void SubRecognizeEditDialog::init(const SubFile &_subFile, const QString &videoFilePath, bool translate)
{
    QPushButton *translatorSettingBtn = new KPushButton("", this);
    translatorSettingBtn->setObjectName(QStringLiteral("TranslatorSetting"));
    translatorSettingBtn->setFont(*GlobalObjects::iconfont);
    translatorSettingBtn->setText(QChar(0xe607));
    ElaComboBox *translatorConfigCombo = new ElaComboBox(this);
    QPushButton *translateBtn = new KPushButton(tr("Translate"), this);

    QLabel *translatorTip = new QLabel(this);
    translatorTip->setOpenExternalLinks(true);
    QPushButton *cancelBtn = new KPushButton(tr("Cancel"), this);

    QWidget *transHContainer = new QWidget(this);
    QHBoxLayout *transHLayout = new QHBoxLayout(transHContainer);
    transHLayout->setContentsMargins(0, 0, 0, 0);
    transHLayout->addWidget(translatorSettingBtn);
    transHLayout->addWidget(translatorConfigCombo);
    transHLayout->addWidget(translateBtn);
    transHLayout->addStretch(1);

    QWidget *transPHContainer = new QWidget(this);
    QHBoxLayout *transPHLayout = new QHBoxLayout(transPHContainer);
    transPHLayout->setContentsMargins(0, 0, 0, 0);
    transPHLayout->addWidget(translatorTip);
    transPHLayout->addWidget(cancelBtn);
    transPHLayout->addStretch(1);

    QStackedWidget *trasnlatorContainer = new QStackedWidget(this);
    trasnlatorContainer->setContentsMargins(0, 0, 0, 0);
    trasnlatorContainer->addWidget(transHContainer);
    trasnlatorContainer->addWidget(transPHContainer);

    ElaComboBox *subTypeCombo = new ElaComboBox(this);
    QPushButton *saveBtn = new KPushButton(tr("Save"), this);
    QPushButton *loadBtn = new KPushButton(tr("Load"), this);

    QTableView *subView = new QTableView(this);
    subView->horizontalHeader()->setStretchLastSection(true);
    subView->setContextMenuPolicy(Qt::CustomContextMenu);

    QGridLayout *subGLayout = new QGridLayout(this);
    subGLayout->addWidget(trasnlatorContainer, 0, 0, 1, 2);
    subGLayout->addWidget(subTypeCombo, 0, 2);
    subGLayout->addWidget(saveBtn, 0, 3);
    subGLayout->addWidget(loadBtn, 0, 4);
    subGLayout->addWidget(subView, 1, 0, 1, 5);

    subGLayout->setColumnStretch(1, 1);
    subGLayout->setRowStretch(1, 1);
    setSizeSettingKey("DialogSize/SubRecognizeEditDialog", QSize(600, 600));

    translatorConfigCombo->setModel(TranslatorConfigManager::instance());
    int defaultTranslator = GlobalObjects::appSetting->value(SETTING_KEY_SUB_TRANSLATOR_DEFAULT, -1).toInt();
    if (defaultTranslator >= 0 && defaultTranslator < TranslatorConfigManager::instance()->rowCount(QModelIndex()))
    {
        translatorConfigCombo->setCurrentIndex(defaultTranslator);
    }

    subTypeCombo->addItems({tr("Bilingual(Translated First)"), tr("Bilingual(Original First)"), tr("Original"), tr("Translated")});
    subTypeCombo->setVisible(translate);

    QSharedPointer<SubFile> subFile = QSharedPointer<SubFile>::create(_subFile);
    SubtitleEditModel *model = new SubtitleEditModel(translate, *subFile, this);
    subView->setModel(model);

    ElaMenu *actionMenu = new ElaMenu(subView);
    QAction *removeAction = actionMenu->addAction(tr("Remove"));
    QAction *translateAction = actionMenu->addAction(tr("Translate"));
    QObject::connect(subView, &QTreeView::customContextMenuRequested, this, [=](){
        if (!subView->selectionModel()->hasSelection()) return;
        actionMenu->exec(QCursor::pos());
    });

    auto submitTranslateTask = [=](SubtitleTranslator *task){
        removeAction->setEnabled(false);
        translateAction->setEnabled(false);
        saveBtn->setEnabled(false);
        loadBtn->setEnabled(false);
        GlobalObjects::appSetting->setValue(SETTING_KEY_SUB_TRANSLATOR_DEFAULT, translatorConfigCombo->currentIndex());
        trasnlatorContainer->setCurrentIndex(1);
        model->setEnableTranslate(true);
        QObject::connect(task, &SubtitleTranslator::infoChanged, this, [=](const QString &content, int flag){
                translatorTip->setText(content);
                if (flag & NM_ERROR)
                {
                    this->showMessage(content, flag);
                }
            }, Qt::QueuedConnection);
        QObject::connect(task, &SubtitleTranslator::progreeUpdated, this, [=](int progress){
                translatorTip->setText(tr("Run Traslating...%1%").arg(progress));
            }, Qt::QueuedConnection);
        QObject::connect(task, &SubtitleRecognizer::statusChanged, this, [=](TaskStatus status){
                if (status == TaskStatus::Cancelled || status == TaskStatus::Failed || status == TaskStatus::Finished)
                {
                    trasnlatorContainer->setCurrentIndex(0);
                    translatorTip->clear();
                    removeAction->setEnabled(true);
                    translateAction->setEnabled(true);
                    saveBtn->setEnabled(true);
                    loadBtn->setEnabled(true);
                    this->task = nullptr;
                    this->showBusyState(false);
                    subTypeCombo->show();
                }
            }, Qt::QueuedConnection);
        TaskPool::instance()->submitTask(task);
        showBusyState(true);
    };


    QObject::connect(removeAction, &QAction::triggered, this, [=](){
        QModelIndexList selection = subView->selectionModel()->selection().indexes();
        if (selection.empty()) return;
        QSet<int> rows;
        for (auto &index : selection) rows.insert(index.row());
        model->removeItems(rows.values());
    });

    QObject::connect(translateAction, &QAction::triggered, this, [=](){
        QModelIndexList selection = subView->selectionModel()->selection().indexes();
        if (selection.empty()) return;

        TranslatorConfigManager *confManager = TranslatorConfigManager::instance();
        if (confManager->rowCount(QModelIndex()) == 0) return;
        TranslatorConfig curConfig = confManager->config(translatorConfigCombo->currentIndex());
        if (!curConfig.isValid())
        {
            showMessage(tr("The translator configuration is invalid. Please set the url/model/prompt/apiKey"), NM_HIDE | NM_ERROR);
            return;
        }

        QSet<int> rows;
        for (auto &index : selection) rows.insert(index.row());
        QList<int> rowsList{rows.values()};
        std::sort(rowsList.begin(), rowsList.end());

        QStringList items;
        items.reserve(rowsList.size());
        for (int row : rowsList)
        {
            items.append(model->data(model->index(row, (int)SubtitleEditModel::Columns::TEXT, QModelIndex()), Qt::DisplayRole).toString());
        }
        task = new SubtitleTranslator(curConfig, items);
        QObject::connect(task, &SubtitleTranslator::subTranslated, this, [=](const QStringList &translatedSubs, int startPos){
                for (int i = 0; i < translatedSubs.size(); ++i)
                {
                    if (i + startPos < rowsList.size())
                    {
                        QModelIndex index = model->index(rowsList[i + startPos], (int)SubtitleEditModel::Columns::TEXT_TRANSLATED, QModelIndex());
                        model->setData(index, translatedSubs[i], Qt::EditRole);
                    }
                }
            }, Qt::QueuedConnection);
        submitTranslateTask(task);
    });


    QObject::connect(translateBtn, &QPushButton::clicked, this, [=](){
        TranslatorConfigManager *confManager = TranslatorConfigManager::instance();
        if (confManager->rowCount(QModelIndex()) == 0) return;
        TranslatorConfig curConfig = confManager->config(translatorConfigCombo->currentIndex());
        if (!curConfig.isValid())
        {
            showMessage(tr("The translator configuration is invalid. Please set the url/model/prompt/apiKey"), NM_HIDE | NM_ERROR);
            return;
        }
        task = new SubtitleTranslator(curConfig, *subFile);
        QObject::connect(task, &SubtitleTranslator::subTranslated, this, [=](const QStringList &translatedSubs, int startPos){
                for (int i = 0; i < translatedSubs.size(); ++i)
                {
                    if (i + startPos < model->rowCount(QModelIndex()))
                    {
                        QModelIndex index = model->index(i + startPos, (int)SubtitleEditModel::Columns::TEXT_TRANSLATED, QModelIndex());
                        model->setData(index, translatedSubs[i], Qt::EditRole);
                    }
                }
            }, Qt::QueuedConnection);
        submitTranslateTask(task);
    });

    QObject::connect(cancelBtn, &QPushButton::clicked, this, [=](){
        if (task)
        {
            task->cancel();
        }
    });

    QObject::connect(translatorSettingBtn, &QPushButton::clicked, this, [=](){
        TranslatorConfigEditDialog configEditor(this);
        configEditor.exec();
    });

    auto saveSub = [=](const QString &path){
        int saveMode = subTypeCombo->currentIndex();
        if (saveMode == 0)
        {
            return model->getTranslatedSubFile(false, true).saveSRT(path);
        }
        else if (saveMode == 1)
        {
            return model->getTranslatedSubFile(false, false).saveSRT(path);
        }
        else if (saveMode == 3)
        {
            return model->getTranslatedSubFile(true).saveSRT(path);
        }
        return subFile->saveSRT(path);
    };

    QObject::connect(saveBtn, &QPushButton::clicked, this, [=](){
        QFileInfo saveFileInfo(videoFilePath);
        QString path = saveFileInfo.absolutePath() + "/" + saveFileInfo.baseName();
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Subtitle"), path, tr("SRT Sub (*.srt)"));
        if (!fileName.isEmpty())
        {
            saveSub(fileName);
        }
    });

    loadBtn->setEnabled(!GlobalObjects::mpvplayer->getCurrentFile().isEmpty());
    QObject::connect(loadBtn, &QPushButton::clicked, this, [=](){
        QFileInfo videoFileInfo(GlobalObjects::mpvplayer->getCurrentFile());
        QString savePath = videoFileInfo.absolutePath() + "/" + videoFileInfo.baseName() + "_kiko.srt";
        int fIndex = 1;
        while (QFile::exists(savePath))
        {
            savePath = QString("%1/%2_kiko_%3.srt").arg(videoFileInfo.absolutePath(), videoFileInfo.baseName()).arg(fIndex++);
        }
        if (saveSub(savePath))
        {
            GlobalObjects::mpvplayer->addSubtitle(savePath);
            CFramelessDialog::onAccept();
        }
        else
        {
            showMessage(tr("Save Failed: %1").arg(savePath), NM_ERROR | NM_HIDE);
        }

    });
}

TranslatorConfigEditDialog::TranslatorConfigEditDialog(QWidget *parent) : CFramelessDialog(tr("Translation Service API Configuration"), parent)
{
    QPushButton *addConfBtn = new KPushButton(tr("Add"), this);
    QPushButton *removeConfBtn = new KPushButton(tr("Remove"), this);
    QListView *confView = new QListView(this);
    confView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    confView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    confView->setViewMode(QListView::ListMode);
    confView->setObjectName(QStringLiteral("TranslatorConfView"));
    confView->setFont(QFont(GlobalObjects::normalFont, 11));
    confView->setFixedWidth(160);

    QLabel *configTip = new QLabel(this);
    configTip->setOpenExternalLinks(true);
    QLabel *urlTip = new QLabel(tr("Service URL"), this);
    ElaLineEdit *urlEdit = new ElaLineEdit(this);

    QLabel *apiKeyTip = new QLabel(tr("API Key"), this);
    ElaLineEdit *apiKeyEdit = new ElaLineEdit(this);

    QLabel *modelTip = new QLabel(tr("Model"), this);
    ElaLineEdit *modelEdit = new ElaLineEdit(this);

    QLabel *batchSizeTip = new QLabel(tr("Number of Subtitle Entries per Translation"), this);
    ElaSpinBox *batchSizeSpin = new ElaSpinBox(this);

    ElaCheckBox *postHistCheck = new ElaCheckBox(tr("Include previous messages when requesting translation"), this);

    QLabel *promptTip = new QLabel(tr("Prompt"), this);
    KPlainTextEdit *promptEdit = new KPlainTextEdit(this);

    QHBoxLayout *confBtnLayout = new QHBoxLayout;
    confBtnLayout->setContentsMargins(0, 0, 0, 0);
    confBtnLayout->addWidget(addConfBtn);
    confBtnLayout->addWidget(removeConfBtn);

    QGridLayout *tGLayout = new QGridLayout(this);
    tGLayout->addLayout(confBtnLayout, 11, 0);
    tGLayout->addWidget(confView, 0, 0, 9, 1);
    tGLayout->addWidget(configTip, 0, 1, 1, 2);
    tGLayout->addWidget(urlTip, 1, 1);
    tGLayout->addWidget(urlEdit, 2, 1, 1, 2);
    tGLayout->addWidget(apiKeyTip, 3, 1);
    tGLayout->addWidget(apiKeyEdit, 4, 1, 1, 2);
    tGLayout->addWidget(modelTip, 5, 1);
    tGLayout->addWidget(modelEdit, 6, 1, 1, 2);
    tGLayout->addWidget(batchSizeTip, 7, 1);
    tGLayout->addWidget(batchSizeSpin, 7, 2);
    tGLayout->addWidget(postHistCheck, 8, 1, 1, 2);
    tGLayout->addWidget(promptTip, 9, 1, 1, 2);
    tGLayout->addWidget(promptEdit, 10, 1, 2, 2);
    tGLayout->setRowStretch(10, 1);
    tGLayout->setColumnStretch(2, 1);
    setSizeSettingKey("DialogSize/TranslatorConfigEditDialog", QSize(400, 600));

    QObject::connect(addConfBtn, &QPushButton::clicked, this, [=](){
        LineInputDialog input(tr("Translator Config"), tr("Name"), "", "DialogSize/AddTranslatorConfig", false, this);
        if (QDialog::Accepted == input.exec())
        {
            TranslatorConfigManager *manager = TranslatorConfigManager::instance();
            manager->addConfig(input.text);
            confView->setCurrentIndex(confView->model()->index(manager->rowCount(QModelIndex()) - 1, 0));
            emit confView->clicked(confView->model()->index(manager->rowCount(QModelIndex()) - 1, 0));
        }
    });

    QObject::connect(removeConfBtn, &QPushButton::clicked, this, [=](){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        if (manager->rowCount(QModelIndex()) <= 1) return;

        QModelIndex index = confView->currentIndex();
        if (index.isValid())
        {
            manager->removeConfig(index.row());
            confView->setCurrentIndex(confView->model()->index(manager->rowCount(QModelIndex()) - 1, 0));
            emit confView->clicked(confView->model()->index(manager->rowCount(QModelIndex()) - 1, 0));
        }
    });

    confView->setModel(TranslatorConfigManager::instance());

    QObject::connect(confView, &QListView::clicked, this, [=](const QModelIndex &index){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();

        batchSizeSpin->blockSignals(true);
        promptEdit->blockSignals(true);

        configTip->setText(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::TIP), Qt::DisplayRole).toString());
        urlEdit->setText(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::URL), Qt::DisplayRole).toString());
        apiKeyEdit->setText(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::API_KEY), Qt::DisplayRole).toString());
        modelEdit->setText(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::MODEL), Qt::DisplayRole).toString());
        batchSizeSpin->setValue(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::BATCH_SIZE), Qt::DisplayRole).toInt());
        postHistCheck->setChecked(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::POST_HISTORY), Qt::DisplayRole).toBool());
        promptEdit->setPlainText(manager->data(index.siblingAtColumn((int)TranslatorConfigManager::Columns::PROMPT), Qt::DisplayRole).toString());

        batchSizeSpin->blockSignals(false);
        promptEdit->blockSignals(false);
    });

    QObject::connect(urlEdit, &ElaLineEdit::textEdited, this, [=](const QString &text){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        QModelIndex index = confView->currentIndex();
        manager->setData(index.siblingAtColumn((int)TranslatorConfigManager::Columns::URL), text, Qt::EditRole);
    });

    QObject::connect(apiKeyEdit, &ElaLineEdit::textEdited, this, [=](const QString &text){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        QModelIndex index = confView->currentIndex();
        manager->setData(index.siblingAtColumn((int)TranslatorConfigManager::Columns::API_KEY), text, Qt::EditRole);
    });

    QObject::connect(modelEdit, &ElaLineEdit::textEdited, this, [=](const QString &text){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        QModelIndex index = confView->currentIndex();
        manager->setData(index.siblingAtColumn((int)TranslatorConfigManager::Columns::MODEL), text, Qt::EditRole);
    });

    QObject::connect(batchSizeSpin, &ElaSpinBox::valueChanged, this, [=](int value){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        QModelIndex index = confView->currentIndex();
        manager->setData(index.siblingAtColumn((int)TranslatorConfigManager::Columns::BATCH_SIZE), value, Qt::EditRole);
    });

    QObject::connect(postHistCheck, &ElaCheckBox::clicked, this, [=](bool checked){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        QModelIndex index = confView->currentIndex();
        manager->setData(index.siblingAtColumn((int)TranslatorConfigManager::Columns::POST_HISTORY), checked, Qt::EditRole);
    });

    QObject::connect(promptEdit, &KPlainTextEdit::textChanged, this, [=](){
        TranslatorConfigManager *manager = TranslatorConfigManager::instance();
        QModelIndex index = confView->currentIndex();
        manager->setData(index.siblingAtColumn((int)TranslatorConfigManager::Columns::PROMPT), promptEdit->toPlainText(), Qt::EditRole);
    });

    emit confView->clicked(confView->model()->index(0, 0));
    confView->setCurrentIndex(confView->model()->index(0, 0));
}
