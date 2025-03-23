#include "mpvconfediror.h"
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QSyntaxHighlighter>
#include <QPlainTextEdit>
#include "UI/widgets/fonticonbutton.h"
#include "Common/notifier.h"
#include "UI/inputdialog.h"
#include "UI/widgets/kplaintextedit.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"

namespace
{
class OptionHighLighter : public QSyntaxHighlighter
{
public:
    OptionHighLighter(QTextDocument *parent):QSyntaxHighlighter(parent)
    {
        optionNameFormat.setForeground(QColor(41,133,199));
        optionContentFormat.setForeground(QColor(245,135,31));
        commentFormat.setForeground(QColor(142,142,142));
        optionRe.setPattern("\\s*([^#=\"]+)=?(.*)\n?");
        commentRe.setPattern("\\s*#.*\n?");
    }
    // QSyntaxHighlighter interface
protected:
    virtual void highlightBlock(const QString &text)
    {
        int index= commentRe.indexIn(text);
        if(index!=-1)
        {
            setFormat(index, commentRe.matchedLength(), commentFormat);
            return;
        }
        index = optionRe.indexIn(text);
        if (index>-1)
        {
            QStringList opList = optionRe.capturedTexts();
            int opNamePos = optionRe.pos(1), opContentPos = optionRe.pos(2);
            if (opNamePos>-1)
            {
                setFormat(opNamePos, opList.at(1).length(), optionNameFormat);
            }
            if (opContentPos>-1)
            {
                setFormat(opContentPos + 1, opList.at(2).length() - 1, optionContentFormat);
            }
        }
    }
private:
    QTextCharFormat optionNameFormat;
    QTextCharFormat optionContentFormat;
    QTextCharFormat commentFormat;
    QRegExp optionRe,commentRe;
};
}
MPVConfEdiror::MPVConfEdiror(QWidget *parent) : CFramelessDialog(tr("MPV Configuration"), parent, true)
{
    QLabel *tipLable = new QLabel(tr("One option per line, without the leading \"--\". "
                                     "Some options take effect after restart"), this);
    tab = new QTabWidget(this);
    tab->setTabsClosable(true);
    tab->setFont(QFont(GlobalObjects::normalFont, 11));

    FontIconButton *addGroup = new FontIconButton(QChar(0xe6f3), "", 14, 10, 2, this);
    addGroup->setObjectName(QStringLiteral("FontIconToolButton"));
    addGroup->setContentsMargins(2, 2, 2, 2);
    tab->setCornerWidget(addGroup, Qt::TopRightCorner);

    QVBoxLayout *dialogVLayout = new QVBoxLayout(this);
    dialogVLayout->addWidget(tipLable);
    dialogVLayout->addWidget(tab);

    QObject::connect(addGroup, &FontIconButton::clicked, this, [this](){
        LineInputDialog input(tr("Add MPV Option Group"), tr("Name"), "", "DialogSize/AddMPVOptionGroup", false, this);
        if(QDialog::Accepted == input.exec())
        {
            for(const OptionGroup &g : optionGroups)
            {
                if(g.groupKey == input.text)
                {
                    showMessage(tr("Group \"%1\" already exists").arg(input.text), NM_ERROR | NM_HIDE);
                    return;
                }
            }
            optionGroups.append(OptionGroup());
            OptionGroup &group = optionGroups.back();
            group.changed = true;
            group.editor = new KPlainTextEdit(tab);
            group.editor->setObjectName(QStringLiteral("MPVConfEditor"));
            group.groupKey = input.text;
            int index = tab->addTab(group.editor, group.groupKey);
            group.editor->setFont(QFont("Consolas", 12));
            new OptionHighLighter(group.editor->document());
            int i = optionGroups.size() - 1;
            QObject::connect(group.editor, &QPlainTextEdit::textChanged, [this, i](){
                if(!optionGroups[i].editor->document()->isModified()) return;
                optionGroups[i].changed = true;
                tab->setTabText(i, "*" + optionGroups[i].groupKey);
            });
            tab->setCurrentIndex(index);
        }
    });

    QObject::connect(tab, &QTabWidget::tabCloseRequested, this, [this](int index){
        if(index == 0) return;
        hasRemoved = true;
        currentRemoved = optionGroups[index].groupKey == GlobalObjects::mpvplayer->currentOptionGroup();
        optionGroups.remove(index);
        tab->removeTab(index);
    });

    loadOptions();
    setSizeSettingKey("DialogSize/MPVConfEdiror", QSize(600, 500));
}

void MPVConfEdiror::onAccept()
{
    bool optionChanged = hasRemoved || currentRemoved;
    for (const OptionGroup &g : optionGroups)
    {
        if (g.changed)
        {
            optionChanged = true;
            break;
        }
    }
    if (!optionChanged)
    {
        CFramelessDialog::onAccept();
    }
    QVector<QStringList> optionsGroupList;
    QStringList optionGroupKeys;
    bool currentChanged = false;
    for (const OptionGroup &g : optionGroups)
    {
        if (g.changed && g.groupKey == GlobalObjects::mpvplayer->currentOptionGroup())
        {
            currentChanged = true;
        }
        if (g.groupKey == "default")
        {
            if (g.changed)
            {
                GlobalObjects::appSetting->setValue("Play/MPVParameters",g.editor->toPlainText());
            }
            continue;
        }
        optionGroupKeys.append(g.groupKey);
        optionsGroupList.append(g.editor->toPlainText().split('\n'));
    }
    GlobalObjects::appSetting->setValue("Play/ParameterGroupKeys", optionGroupKeys);
    GlobalObjects::appSetting->setValue("Play/MPVParameterGroups", QVariant::fromValue(optionsGroupList));
    GlobalObjects::mpvplayer->loadOptions();
    if (currentRemoved)
    {
        GlobalObjects::mpvplayer->setOptionGroup("default");
        Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tr("Option Group \"%1\" has been removed, switch to default").arg(GlobalObjects::mpvplayer->currentOptionGroup()));
    }
    else if (currentChanged)
    {
        GlobalObjects::mpvplayer->setOptionGroup(GlobalObjects::mpvplayer->currentOptionGroup());
        Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, tr("Reload Option Group \"%1\"").arg(GlobalObjects::mpvplayer->currentOptionGroup()));
    }
    CFramelessDialog::onAccept();
}

void MPVConfEdiror::loadOptions()
{
    QFile defaultOptionFile(":/res/mpvOptions");
    defaultOptionFile.open(QFile::ReadOnly);
    QStringList defaultOptions = GlobalObjects::appSetting->value("Play/MPVParameters", QString(defaultOptionFile.readAll())).toString().split('\n');
    QVector<QStringList> optionsGroupList = GlobalObjects::appSetting->value("Play/MPVParameterGroups").value<QVector<QStringList>>();
    optionsGroupList.prepend(defaultOptions);

    QString curOptionGroup = GlobalObjects::appSetting->value("Play/DefaultParameterGroup", "default").toString();
    QStringList optionGroupKeys = GlobalObjects::appSetting->value("Play/ParameterGroupKeys").toStringList();
    optionGroupKeys.prepend("default");

    if(optionGroupKeys.size() < optionsGroupList.size())
    {
        optionsGroupList.resize(optionGroupKeys.size());
    }
    optionGroups.resize(optionGroupKeys.size());
    QFont font("Consolas", 12);
    for(int i = 0; i < optionGroupKeys.size(); ++i)
    {
        OptionGroup &group = optionGroups[i];
        group.changed = false;
        group.editor = new KPlainTextEdit(tab);
        group.editor->setObjectName(QStringLiteral("MPVConfEditor"));
        group.groupKey = optionGroupKeys[i];
        tab->addTab(group.editor, group.groupKey);
        QObject::connect(group.editor, &QPlainTextEdit::textChanged, [this, i](){
            if(!optionGroups[i].editor->document()->isModified()) return;
            optionGroups[i].changed = true;
            tab->setTabText(i, "*" + optionGroups[i].groupKey);
        });
        group.editor->blockSignals(true);
        group.editor->setPlainText(optionsGroupList[i].join('\n'));
        group.editor->setFont(font);
        new OptionHighLighter(group.editor->document());

        group.editor->blockSignals(false);
    }
}
