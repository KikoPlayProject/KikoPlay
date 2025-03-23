#include "trackersubscribedialog.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaMenu.h"
#include "Download/trackersubscriber.h"
#include "UI/inputdialog.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/kpushbutton.h"

#include <QTreeView>
#include <QAction>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>


TrackerSubscribeDialog::TrackerSubscribeDialog(QWidget *parent) : CFramelessDialog(tr("Tracker Subscribe"), parent)
{
    QPushButton *addTrackerSource = new KPushButton(tr("Add"), this);
    QPushButton *checkAll = new KPushButton(tr("Check All"), this);
    QCheckBox *autoCheck = new ElaCheckBox(tr("Auto Check"), this);
    autoCheck->setChecked(TrackerSubscriber::subscriber()->isAutoCheck());
    QTreeView *trackerSrcView = new QTreeView(this);
    trackerSrcView->setRootIsDecorated(false);
    trackerSrcView->setAlternatingRowColors(true);
    trackerSrcView->setModel(TrackerSubscriber::subscriber());
    trackerSrcView->setContextMenuPolicy(Qt::CustomContextMenu);
    trackerSrcView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QPlainTextEdit *trackerText = new KPlainTextEdit(this);
    trackerText->setReadOnly(true);
    trackerText->hide();

    QGridLayout *tsGLayout = new QGridLayout(this);
    tsGLayout->addWidget(addTrackerSource, 0, 0);
    tsGLayout->addWidget(autoCheck, 0, 2);
    tsGLayout->addWidget(checkAll, 0, 3);
    tsGLayout->addWidget(trackerSrcView, 1, 0, 1, 4);
    tsGLayout->addWidget(trackerText, 2, 0, 1, 4);
    tsGLayout->setRowStretch(1, 1);
    tsGLayout->setColumnStretch(1, 1);
    setSizeSettingKey("DialogSize/TrackerSubscribe", QSize(400, 300));
    trackerSrcView->header()->resizeSection(0, width()/2);


    QObject::connect(addTrackerSource, &QPushButton::clicked, this, [=](){
        LineInputDialog input(tr("Subscirbe"), tr("URL"), "", "DialogSize/AddTrackerSubscribe", false, this);
        if(QDialog::Accepted == input.exec())
        {
            TrackerSubscriber::subscriber()->add(input.text);
        }
    });
    QObject::connect(checkAll, &QPushButton::clicked, this, [=](){
        TrackerSubscriber::subscriber()->check(-1);
    });
    QObject::connect(autoCheck, &QCheckBox::stateChanged, this,[](int state){
        TrackerSubscriber::subscriber()->setAutoCheck(state == Qt::CheckState::Checked);
    });

    ElaMenu *actionMenu = new ElaMenu(trackerSrcView);
    QObject::connect(trackerSrcView, &QTreeView::customContextMenuRequested, this, [=](){
        if (!trackerSrcView->selectionModel()->hasSelection()) return;
        actionMenu->exec(QCursor::pos());
    });
    QAction *actCopyURL = actionMenu->addAction(tr("Copy URL"));
    QObject::connect(actCopyURL, &QAction::triggered, this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if (selection.empty()) return;
        const QString url(selection.first().siblingAtColumn(static_cast<int>(TrackerSubscriber::Columns::URL)).data().toString());
        QClipboard *cb = QApplication::clipboard();
        cb->setText(url);
    });
    QAction *actRemoveSubscirbe = actionMenu->addAction(tr("Remove Subscribe"));
    QObject::connect(actRemoveSubscirbe, &QAction::triggered, this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if (selection.empty()) return;
        TrackerSubscriber::subscriber()->remove(selection.first().row());
    });
    QAction *actCheck = actionMenu->addAction(tr("Check"));
    QObject::connect(actCheck, &QAction::triggered, this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if (selection.empty()) return;
        TrackerSubscriber::subscriber()->check(selection.first().row());
    });

    QObject::connect(trackerSrcView->selectionModel(), &QItemSelectionModel::selectionChanged,this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if (selection.empty())
        {
            trackerText->clear();
            trackerText->hide();
            tsGLayout->setRowStretch(2, 0);
        }
        else
        {
            QStringList trackers = TrackerSubscriber::subscriber()->getTrackers(selection.first().row());
            trackerText->setPlainText(trackers.join('\n'));
            trackerText->show();
            tsGLayout->setRowStretch(2, 1);
        }

    });
    QObject::connect(TrackerSubscriber::subscriber(), &TrackerSubscriber::checkStateChanged, this, [=](bool checking){
        addTrackerSource->setEnabled(!checking);
        checkAll->setEnabled(!checking);
        autoCheck->setEnabled(!checking);
        trackerSrcView->setEnabled(!checking);
    });
    QObject::connect(TrackerSubscriber::subscriber(), &TrackerSubscriber::trackerListChanged, this, [=](int index){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if (selection.empty() || selection.first().row() != index) return;
        QStringList trackers = TrackerSubscriber::subscriber()->getTrackers(selection.first().row());
        trackerText->setPlainText(trackers.join('\n'));
        trackerText->show();
        tsGLayout->setRowStretch(2, 1);
    });
}
