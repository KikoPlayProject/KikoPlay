#include "logwindow.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QStackedLayout>
#include "globalobjects.h"
#include "Script/scriptlogger.h"
#include "Play/Video/mpvplayer.h"
LogWindow::LogWindow(QWidget *parent) : CFramelessDialog(tr("Log"),parent,false,true,false)
{
    QPlainTextEdit *mpvLogView=new QPlainTextEdit(this);
    mpvLogView->setReadOnly(true);
    mpvLogView->setMaximumBlockCount(1024);
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::showLog,[mpvLogView](const QString &log){
        mpvLogView->appendPlainText(log.trimmed());
        QTextCursor cursor = mpvLogView->textCursor();
        cursor.movePosition(QTextCursor::End);
        mpvLogView->setTextCursor(cursor);
    });

    QPlainTextEdit *scriptLogView=new QPlainTextEdit(this);
    scriptLogView->setReadOnly(true);
    scriptLogView->setMaximumBlockCount(1024);
    for(const QString &log: ScriptLogger::instance()->getLogs())
    {
        scriptLogView->appendPlainText(log);
    }

    QObject::connect(ScriptLogger::instance(),&ScriptLogger::logging,[=](const QString &log){
        scriptLogView->appendPlainText(log.trimmed());
        QTextCursor cursor = scriptLogView->textCursor();
        cursor.movePosition(QTextCursor::End);
        scriptLogView->setTextCursor(cursor);
    });

    logTypeCombo = new QComboBox(this);
    logTypeCombo->addItems({"MPV", "Script"});

    QStackedLayout *logSLayout = new QStackedLayout;
    logSLayout->addWidget(mpvLogView);
    logSLayout->addWidget(scriptLogView);
    logSLayout->setContentsMargins(0, 0, 0, 0);
    QObject::connect(logTypeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, logSLayout, &QStackedLayout::setCurrentIndex);
    logTypeCombo->setCurrentIndex(0);

    QPushButton *cleanLog=new QPushButton(tr("Clean"),this);
    cleanLog->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    QObject::connect(cleanLog,&QPushButton::clicked,this,[=](){
        QPlainTextEdit *logEdits[] = {mpvLogView, scriptLogView};
        logEdits[logSLayout->currentIndex()]->clear();
    });

    QGridLayout *logGLayout = new QGridLayout(this);
    logGLayout->addWidget(logTypeCombo, 0, 0);
    logGLayout->addWidget(cleanLog, 0, 1);
    logGLayout->addLayout(logSLayout, 1, 0, 1, 2);
    logGLayout->setRowStretch(1, 1);
    logGLayout->setColumnStretch(1, 1);
}

void LogWindow::show(LogWindow::LogType lt)
{
    logTypeCombo->setCurrentIndex(lt);
    CFramelessDialog::show();
}
