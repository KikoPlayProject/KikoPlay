#include "logwindow.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QStackedLayout>
#include <QLineEdit>
#include "globalobjects.h"
#include "Common/logger.h"
#include "Play/Video/mpvplayer.h"
LogWindow::LogWindow(QWidget *parent) : CFramelessDialog(tr("Log"),parent,false,true,false)
{
    logTypeCombo = new QComboBox(this);
    logTypeCombo->addItems(Logger::logger()->LogTypeNames);

    QVector<QPlainTextEdit *> logEdits((int)Logger::LogType::UNKNOWN);
    QFont logFont("Consolas", 10);
    for(int i = 0; i < Logger::LogType::UNKNOWN; ++i)
    {
        logEdits[i] = new QPlainTextEdit(this);
        logEdits[i]->setReadOnly(true);
        logEdits[i]->setFont(logFont);
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

    mpvPropViewer = new MPVPropertyViewer(this);
    QPushButton *viewProperty = new QPushButton("MPV Property Viewer", this);
    viewProperty->setVisible(false);
    QObject::connect(logTypeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [=](int idx){
        viewProperty->setVisible(idx == static_cast<int>(Logger::LogType::MPV));
    });
    QObject::connect(viewProperty, &QPushButton::clicked, this, [=](){
        mpvPropViewer->show();
    });

    QPushButton *cleanLog=new QPushButton(tr("Clean"),this);
    cleanLog->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    QObject::connect(cleanLog,&QPushButton::clicked,this,[=](){
        logEdits[logSLayout->currentIndex()]->clear();
    });

    QGridLayout *logGLayout = new QGridLayout(this);
    logGLayout->setContentsMargins(0, 0, 0, 0);
    logGLayout->addWidget(logTypeCombo, 0, 0);
    logGLayout->addWidget(viewProperty, 0, 2);
    logGLayout->addWidget(cleanLog, 0, 3);
    logGLayout->addLayout(logSLayout, 1, 0, 1, 4);
    logGLayout->setRowStretch(1, 1);
    logGLayout->setColumnStretch(1, 1);
    setSizeSettingKey("DialogSize/LogWindow",QSize(400*logicalDpiX()/96,300*logicalDpiY()/96));
}

void LogWindow::show(Logger::LogType lt)
{
    logTypeCombo->setCurrentIndex(lt);
    CFramelessDialog::show();
}

MPVPropertyViewer::MPVPropertyViewer(QWidget *parent) : CFramelessDialog("MPV Property Viewer",parent,false,true,false)
{
    QLineEdit *propertyEdit = new QLineEdit(this);
    QPlainTextEdit *propertyContent = new QPlainTextEdit(this);
    propertyContent->setReadOnly(true);
    propertyContent->setFont(QFont("Consolas", 10));

    QGridLayout *viewGLayout = new QGridLayout(this);
    viewGLayout->setContentsMargins(0, 0, 0, 0);
    viewGLayout->addWidget(propertyEdit, 0, 0);
    viewGLayout->addWidget(propertyContent, 1, 0);
    viewGLayout->setRowStretch(1, 1);
    viewGLayout->setColumnStretch(0, 1);
    setSizeSettingKey("DialogSize/MPVPropertyViewer",QSize(300*logicalDpiX()/96,300*logicalDpiY()/96));

    QObject::connect(propertyEdit, &QLineEdit::editingFinished, this, [=](){
        const QString property = propertyEdit->text().trimmed();
        if(property.isEmpty()) return;
        int errCode = 0;
        QVariant val = GlobalObjects::mpvplayer->getMPVPropertyVariant(property, errCode);
        propertyContent->clear();
        if(errCode < 0)
        {
            propertyContent->appendPlainText(QString("Get Property [%1] error: %2").arg(property).arg(errCode));
            return;
        }
        if(val.canConvert(QMetaType::QString))
        {
            propertyContent->appendPlainText(QString("Property [%1] value: %2").arg(property).arg(val.toString()));
        }
        else
        {
            QJsonDocument d = QJsonDocument::fromVariant(val);
            propertyContent->appendPlainText(QString("Property [%1] value: ").arg(property));
            propertyContent->appendPlainText(d.toJson(QJsonDocument::Indented));
        }
    });
}
