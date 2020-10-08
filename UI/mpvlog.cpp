#include "mpvlog.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
MPVLog::MPVLog(QWidget *parent) : CFramelessDialog(tr("MPV Log"),parent,false,true,false)
{
    QPlainTextEdit *logView=new QPlainTextEdit(this);
    logView->setReadOnly(true);
    logView->setMaximumBlockCount(1024);
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::showLog,[logView](const QString &log){
        logView->appendPlainText(log.trimmed());
        QTextCursor cursor = logView->textCursor();
        cursor.movePosition(QTextCursor::End);
        logView->setTextCursor(cursor);
    });

    QPushButton *cleanLog=new QPushButton(tr("Clean"),this);
    cleanLog->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    QObject::connect(cleanLog,&QPushButton::clicked,logView,&QPlainTextEdit::clear);

    QVBoxLayout *logVLayout=new QVBoxLayout(this);
    logVLayout->addWidget(cleanLog);
    logVLayout->addWidget(logView);
}
