#include "logwindow.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QStackedLayout>
#include "globalobjects.h"
#include "Common/logger.h"
#include "Play/Video/mpvplayer.h"
LogWindow::LogWindow(QWidget *parent) : CFramelessDialog(tr("Log"),parent,false,true,false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    logTypeCombo = new QComboBox(this);
    logTypeCombo->addItems(Logger::logger()->LogTypeNames);

    QVector<QPlainTextEdit *> logEdits((int)Logger::LogType::UNKNOWN);
    for(int i = 0; i < Logger::LogType::UNKNOWN; ++i)
    {
        logEdits[i] = new QPlainTextEdit(this);
        logEdits[i]->setReadOnly(true);
        logEdits[i]->setMaximumBlockCount(Logger::logger()->bufferSize);
        for(const QString &log: Logger::logger()->getLogs(Logger::LogType(i)))
        {
            logEdits[i]->appendPlainText(log);
        }
    }

    QObject::connect(Logger::logger(),&Logger::logAppend, this, [=](Logger::LogType type, const QString &log){
        if(type != Logger::UNKNOWN)
        {
            logEdits[type]->appendPlainText(log);
            QTextCursor cursor = logEdits[type]->textCursor();
            cursor.movePosition(QTextCursor::End);
            logEdits[type]->setTextCursor(cursor);
        }
    });

    QStackedLayout *logSLayout = new QStackedLayout;
    for(auto logView : logEdits)
    {
        logSLayout->addWidget(logView);
    }
    logSLayout->setContentsMargins(0, 0, 0, 0);
    QObject::connect(logTypeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, logSLayout, &QStackedLayout::setCurrentIndex);
    logTypeCombo->setCurrentIndex(0);

    QPushButton *cleanLog=new QPushButton(tr("Clean"),this);
    cleanLog->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    QObject::connect(cleanLog,&QPushButton::clicked,this,[=](){
        logEdits[logSLayout->currentIndex()]->clear();
    });

    QGridLayout *logGLayout = new QGridLayout(this);
    logGLayout->setContentsMargins(0, 0, 0, 0);
    logGLayout->addWidget(logTypeCombo, 0, 0);
    logGLayout->addWidget(cleanLog, 0, 1);
    logGLayout->addLayout(logSLayout, 1, 0, 1, 2);
    logGLayout->setRowStretch(1, 1);
    logGLayout->setColumnStretch(1, 1);
    setSizeSettingKey("DialogSize/LogWindow",QSize(400*logicalDpiX()/96,300*logicalDpiY()/96));
}

void LogWindow::show(Logger::LogType lt)
{
    logTypeCombo->setCurrentIndex(lt);
    CFramelessDialog::show();
}
