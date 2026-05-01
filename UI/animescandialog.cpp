#include "animescandialog.h"
#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QListWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QFileInfo>
#include "Common/notifier.h"
#include "Common/taskpool.h"
#include "Play/Video/mpvmediainfo.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/widgets/elidedlabel.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/kpushbutton.h"

AnimeScanDialog::AnimeScanDialog(const QString &initialDir, QWidget *parent)
    : CFramelessDialog(tr("Scan Folder"), parent)
{
    QFont f = font();
    f.setPointSize(12);

    QLabel *dirTip = new QLabel(tr("Directories"), this);
    dirListWidget = new QListWidget(this);
    dirListWidget->setObjectName(QStringLiteral("ScanFolderView"));
    dirListWidget->addItem(initialDir);
    dirListWidget->setFont(f);

    addDirBtn = new KPushButton(tr("Add"), this);
    removeDirBtn = new KPushButton(tr("Remove"), this);

    enableDurationCheck = new ElaCheckBox(tr("Duration Filter"), this);
    QLabel *minTip = new QLabel(tr("Min(s)"), this);
    minDurationSpin = new ElaSpinBox(this);
    minDurationSpin->setRange(0, 99999);
    minDurationSpin->setValue(0);
    minDurationSpin->setEnabled(false);
    QLabel *maxTip = new QLabel(tr("Max(s)"), this);
    maxDurationSpin = new ElaSpinBox(this);
    maxDurationSpin->setRange(0, 99999);
    maxDurationSpin->setValue(0);
    maxDurationSpin->setEnabled(false);

    QLabel *filterTip = new QLabel(tr("Filename Exclude(one regex per line)"), this);
    filenameFilterEdit = new KPlainTextEdit(this, false);
    filenameFilterEdit->setMaximumHeight(80);
    filenameFilterEdit->setFont(f);

    progressTip = new ElidedLabel(this);
    progressTip->setFontColor(QColor(240, 240, 240));

    startBtn = new KPushButton(tr("Start Scan"), this);

    QWidget *configWidget = new QWidget(this);
    QHBoxLayout *dirBtnLayout = new QHBoxLayout;
    dirBtnLayout->addWidget(dirTip);
    dirBtnLayout->addStretch(1);
    dirBtnLayout->addWidget(addDirBtn);
    dirBtnLayout->addWidget(removeDirBtn);

    QHBoxLayout *durationLayout = new QHBoxLayout;
    durationLayout->addWidget(enableDurationCheck);
    durationLayout->addWidget(minTip);
    durationLayout->addWidget(minDurationSpin);
    durationLayout->addWidget(maxTip);
    durationLayout->addWidget(maxDurationSpin);
    durationLayout->addStretch(1);

    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    configLayout->setContentsMargins(0, 0, 0, 0);
    configLayout->addLayout(dirBtnLayout);
    configLayout->addWidget(dirListWidget, 1);
    configLayout->addLayout(durationLayout);
    configLayout->addWidget(filterTip);
    configLayout->addWidget(filenameFilterEdit);

    resultView = new QTextEdit(this);
    resultView->setReadOnly(true);
    resultView->setFont(f);

    stackedLayout = new QStackedLayout;
    stackedLayout->addWidget(configWidget);
    stackedLayout->addWidget(resultView);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(progressTip, 1);
    bottomLayout->addWidget(startBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(stackedLayout, 1);
    mainLayout->addLayout(bottomLayout);

    setSizeSettingKey("DialogSize/AnimeScanDialog", QSize(560, 400));

    QObject::connect(enableDurationCheck, &QCheckBox::toggled, this, [this](bool checked){
        minDurationSpin->setEnabled(checked);
        maxDurationSpin->setEnabled(checked);
    });

    QObject::connect(addDirBtn, &QPushButton::clicked, this, &AnimeScanDialog::addDirectory);
    QObject::connect(removeDirBtn, &QPushButton::clicked, this, &AnimeScanDialog::removeSelectedDirectory);
    QObject::connect(startBtn, &QPushButton::clicked, this, &AnimeScanDialog::startScan);
}

void AnimeScanDialog::addDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"), "");
    if (dir.isEmpty()) return;
    for (int i = 0; i < dirListWidget->count(); ++i)
    {
        if (dirListWidget->item(i)->text() == dir) return;
    }
    dirListWidget->addItem(dir);
}

void AnimeScanDialog::removeSelectedDirectory()
{
    auto items = dirListWidget->selectedItems();
    for (auto item : items)
    {
        delete dirListWidget->takeItem(dirListWidget->row(item));
    }
}

void AnimeScanDialog::startScan()
{
    if (task)
    {
        task->cancel();
        startBtn->setEnabled(false);
        return;
    }

    if (stackedLayout->currentIndex() == 1)
    {
        stackedLayout->setCurrentIndex(0);
        startBtn->setText(tr("Start Scan"));
        progressTip->setText("");
        return;
    }

    if (dirListWidget->count() == 0)
    {
        showMessage(tr("Please add at least one directory"), NM_HIDE | NM_ERROR);
        return;
    }

    AnimeScanOptions options;
    for (int i = 0; i < dirListWidget->count(); ++i)
    {
        options.directories.append(dirListWidget->item(i)->text());
    }
    options.enableDurationFilter = enableDurationCheck->isChecked();
    options.minDurationSec = minDurationSpin->value();
    options.maxDurationSec = maxDurationSpin->value();
    options.filenameFilters = filenameFilterEdit->toPlainText().split('\n', Qt::SkipEmptyParts);

    if (options.enableDurationFilter && !mediaInfo)
    {
        mediaInfo = new MPVMediaInfo();
    }

    setUIEnabled(false);
    startBtn->setText(tr("Stop"));

    task = new AnimeScanTask(options, options.enableDurationFilter ? mediaInfo : nullptr);
    QObject::connect(task, &AnimeScanTask::infoChanged, this, [this](const QString &info, int){
        progressTip->setText(info);
    }, Qt::QueuedConnection);
    QObject::connect(task, &AnimeScanTask::progreeUpdated, this, [this](int progress){
        progressTip->setText(tr("Progress: %1%").arg(progress));
    }, Qt::QueuedConnection);
    QObject::connect(task, &AnimeScanTask::scanFinished, this, [this](AnimeScanResult result){
        showResults(result);
    }, Qt::QueuedConnection);
    QObject::connect(task, &AnimeScanTask::statusChanged, this, [this](TaskStatus status){
        if (status == TaskStatus::Cancelled || status == TaskStatus::Failed || status == TaskStatus::Finished || status == TaskStatus::PartiallyFinished)
        {
            setUIEnabled(true);
            if (status == TaskStatus::Cancelled || status == TaskStatus::Failed)
            {
                startBtn->setText(tr("Start Scan"));
                if (status == TaskStatus::Cancelled) progressTip->setText("");
            }
            startBtn->setEnabled(true);
            task = nullptr;
            showBusyState(false);
        }
    }, Qt::QueuedConnection);

    TaskPool::instance()->submitTask(task);
    showBusyState(true);
}

AnimeScanDialog::~AnimeScanDialog()
{
    if (mediaInfo)
    {
        QMetaObject::invokeMethod(mediaInfo, [this](){
            mediaInfo->moveToThread(this->thread());
        }, Qt::BlockingQueuedConnection);
        mediaInfo->deleteLater();
        mediaInfo = nullptr;
    }
}

void AnimeScanDialog::setUIEnabled(bool enabled)
{
    addDirBtn->setEnabled(enabled);
    removeDirBtn->setEnabled(enabled);
    dirListWidget->setEnabled(enabled);
    enableDurationCheck->setEnabled(enabled);
    minDurationSpin->setEnabled(enabled && enableDurationCheck->isChecked());
    maxDurationSpin->setEnabled(enabled && enableDurationCheck->isChecked());
    filenameFilterEdit->setEnabled(enabled);
}

void AnimeScanDialog::showResults(const AnimeScanResult &result)
{
    struct EpRecord { QString epString; QString fileName; };
    QMap<QString, QVector<EpRecord>> newAnimeMap;
    QMap<QString, QVector<EpRecord>> existingAnimeMap;
    QMap<ScanFileRecord::FilterReason, QStringList> filteredMap;
    QStringList failedList;

    for (const auto &record : result.records)
    {
        const QString fileName = QFileInfo(record.filePath).fileName().toHtmlEscaped();
        switch (record.status)
        {
        case ScanFileRecord::NewAnime:
            newAnimeMap[record.animeName].append({record.epString.toHtmlEscaped(), fileName});
            break;
        case ScanFileRecord::ExistingAnime:
            existingAnimeMap[record.animeName].append({record.epString.toHtmlEscaped(), fileName});
            break;
        case ScanFileRecord::Filtered:
            filteredMap[record.filterReason].append(fileName);
            break;
        case ScanFileRecord::Failed:
            failedList.append(fileName);
            break;
        }
    }

    QString html;
    html += "<p style='color:#e0e0e0; font-size:13pt;'>"
            + tr("Scan complete: <b>%1</b> total, <span style='color:#4cd964;'>%2</span> matched, "
                 "<span style='color:#ff6b6b;'>%3</span> failed, %4 filtered")
                  .arg(result.totalFiles).arg(result.matchedFiles)
                  .arg(result.failedFiles).arg(result.skippedFiles)
            + "</p>";

    if (!newAnimeMap.isEmpty())
    {
        html += QString("<h3 style='color:#4cd964; margin-bottom:4px;'>%1 (%2)</h3>")
                    .arg(tr("New Anime")).arg(newAnimeMap.size());
        html += "<div style='margin-left:8px; color:#d0d0d0;'>";
        for (auto it = newAnimeMap.constBegin(); it != newAnimeMap.constEnd(); ++it)
        {
            html += QString("<p style='margin:4px 0 2px 0;'><span style='color:#60d0fc;'>%1</span></p>").arg(it.key().toHtmlEscaped());
            for (const auto &ep : it.value())
            {
                html += QString("<p style='margin:1px 0 1px 16px; font-size:small; color:#a0a0a0;'>%1 &nbsp; <span style='color:#707070;'>%2</span></p>")
                            .arg(ep.epString, ep.fileName);
            }
        }
        html += "</div><br>";
    }

    if (!existingAnimeMap.isEmpty())
    {
        html += QString("<h3 style='color:#60d0fc; margin-bottom:4px;'>%1 (%2)</h3>")
                    .arg(tr("Existing Anime / New Episodes")).arg(existingAnimeMap.size());
        html += "<div style='margin-left:8px; color:#d0d0d0;'>";
        for (auto it = existingAnimeMap.constBegin(); it != existingAnimeMap.constEnd(); ++it)
        {
            html += QString("<p style='margin:4px 0 2px 0;'><span style='color:#60d0fc;'>%1</span></p>").arg(it.key().toHtmlEscaped());
            for (const auto &ep : it.value())
            {
                html += QString("<p style='margin:1px 0 1px 16px; font-size:small; color:#a0a0a0;'>%1 &nbsp; <span style='color:#707070;'>%2</span></p>")
                            .arg(ep.epString, ep.fileName);
            }
        }
        html += "</div><br>";
    }

    if (!failedList.isEmpty())
    {
        html += QString("<h3 style='color:#ff6b6b; margin-bottom:4px;'>%1 (%2)</h3>")
                    .arg(tr("Match Failed")).arg(failedList.size());
        html += "<div style='margin-left:8px;'>";
        for (const auto &name : failedList)
        {
            html += QString("<p style='margin:1px 0; font-size:small; color:#a0a0a0;'>%1</p>").arg(name);
        }
        html += "</div><br>";
    }

    if (!filteredMap.isEmpty())
    {
        int totalFiltered = 0;
        for (const auto &list : filteredMap) totalFiltered += list.size();
        html += QString("<h3 style='color:#909090; margin-bottom:4px;'>%1 (%2)</h3>")
                    .arg(tr("Filtered")).arg(totalFiltered);
        html += "<div style='margin-left:8px;'>";

        const QMap<ScanFileRecord::FilterReason, QString> reasonNames = {
            {ScanFileRecord::FilenameExcluded, tr("Filename Excluded")},
            {ScanFileRecord::AlreadyInLibrary, tr("Already in Library")},
            {ScanFileRecord::DurationOutOfRange, tr("Duration Out of Range")},
        };
        for (auto it = filteredMap.constBegin(); it != filteredMap.constEnd(); ++it)
        {
            html += QString("<p style='margin:4px 0 2px 0; font-size:small; color:#a0a0a0;'>%1 (%2)</p>")
                        .arg(reasonNames.value(it.key(), tr("Other"))).arg(it.value().size());
            for (const auto &name : it.value())
            {
                html += QString("<p style='margin:1px 0 1px 16px; font-size:small; color:#707070;'>%1</p>").arg(name);
            }
        }
        html += "</div><br>";
    }

    resultView->setHtml(html);
    stackedLayout->setCurrentIndex(1);
    startBtn->setText(tr("Back"));
}
