#include "keyactionpage.h"
#include <QTreeView>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include "Common/keyactionmodel.h"
#include "Common/keyaction.h"
#include "Common/notifier.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaSpinBox.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"

KeyActionPage::KeyActionPage(QWidget *parent) : SettingPage(parent)
{
    QTreeView *shortcutView = new QTreeView(this);
    shortcutView->setRootIsDecorated(false);
    shortcutView->setFont(QFont(GlobalObjects::normalFont, 11));
    shortcutView->setItemDelegate(new KTreeviewItemDelegate(shortcutView));
    shortcutView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    shortcutView->setModel(KeyActionModel::instance());
    shortcutView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    new FloatScrollBar(shortcutView->verticalScrollBar(), shortcutView);
    shortcutView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    shortcutView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    KPushButton *addBtn = new KPushButton(tr("Add"), this);
    KPushButton *importBtn = new KPushButton(tr("Import input.conf"), this);

    QGridLayout *shortcutGLayout = new QGridLayout(this);
    shortcutGLayout->addWidget(addBtn, 0, 0);
    shortcutGLayout->addWidget(importBtn, 0, 1);
    shortcutGLayout->addWidget(shortcutView, 1, 0, 1, 3);
    shortcutGLayout->setRowStretch(1, 1);
    shortcutGLayout->setColumnStretch(2, 1);

    ElaMenu *actionMenu = new ElaMenu(shortcutView);
    QObject::connect(shortcutView, &QTreeView::customContextMenuRequested, this, [=](){
        if (!shortcutView->selectionModel()->hasSelection()) return;
        actionMenu->exec(QCursor::pos());
    });
    QAction *actRemove = actionMenu->addAction(tr("Remove"));
    QObject::connect(actRemove, &QAction::triggered, this, [=](){
        auto selection = shortcutView->selectionModel()->selectedRows((int)KeyActionModel::Columns::KEY);
        if (selection.empty()) return;
        std::sort(selection.begin(), selection.end(), [](const QModelIndex &a, const QModelIndex &b){
            return a.row() >= b.row();
        });
        for (const QModelIndex &index : selection)
        {
            KeyActionModel::instance()->removeKeyAction(index);
        }
    });
    QAction *actModify = actionMenu->addAction(tr("Modify"));
    auto modifyKey = [=](const QModelIndex &index){
        KeyActionEditDialog addKey(KeyActionModel::instance()->getActionItem(index).get(), this);
        if (QDialog::Accepted == addKey.exec())
        {
            KeyActionModel::instance()->updateKeyAction(index, addKey.key, addKey.triggerType, addKey.action);
            addKey.action = nullptr;
        }
    };
    QObject::connect(actModify, &QAction::triggered, this, [=](){
        auto selection = shortcutView->selectionModel()->selectedRows((int)KeyActionModel::Columns::KEY);
        if (selection.empty())return;
        QModelIndex index(selection.last());
        if (!index.isValid()) return;
        modifyKey(index);
    });
    QObject::connect(shortcutView, &QTreeView::doubleClicked, this, modifyKey);

    QObject::connect(addBtn, &QPushButton::clicked, this, [=](){
        KeyActionEditDialog addKey(nullptr, this);
        if (QDialog::Accepted == addKey.exec())
        {
            KeyActionModel::instance()->addKeyAction(addKey.key, addKey.triggerType, addKey.action);
            addKey.action = nullptr;
        }
    });

    QObject::connect(importBtn, &QPushButton::clicked, this, [=](){
        const QString fileName = QFileDialog::getOpenFileName(this, tr("Select Input Conf"), "",
                                                        "Conf Files(*.conf);;All Files(*)");
        if (fileName.isEmpty()) return;
        laodInputConf(fileName);
    });
}

void KeyActionPage::laodInputConf(const QString &filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    int validCount = 0;
    QTextStream in(&file);
    //in.setCodec("UTF-8");
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        QString comment;
        int commentPos = line.indexOf('#');

        if (commentPos != -1)
        {
            comment = line.mid(commentPos + 1).trimmed();
            line = line.left(commentPos).trimmed();
        }

        if (line.isEmpty()) continue;

        int spacePos = line.indexOf(' ');
        if (spacePos == -1) continue;


        QString key = line.left(spacePos);
        QString command = line.mid(spacePos + 1).trimmed();

        QKeySequence keySeq(key);
        if (key.isEmpty() || command.isEmpty() || keySeq.isEmpty()) continue;


        KeyAction *action = KeyAction::createAction(KeyAction::ActionType::ACT_MPV_COMMAND);
        action->actParams[0].val = command;
        if (!comment.isEmpty()) action->actParams[1].val = comment;
        validCount += KeyActionModel::instance()->addKeyAction(keySeq.toString(), KeyActionItem::ActionTrigger::KEY_PRESS, action);
    }
    emit showMessage(tr("Add %1 Key Action(s)").arg(validCount));
}

KeyActionEditDialog::KeyActionEditDialog(const KeyActionItem *item, QWidget *parent) :
    CFramelessDialog(tr("Edit Key Action"), parent, true), triggerType(KeyActionItem::KEY_PRESS), action(nullptr), refItem(item)
{
    QLabel *shortcutTip = new QLabel(tr("Shortcut Key"), this);
    QKeySequenceEdit *keyEdit = new QKeySequenceEdit(this);
    QLabel *actionTypeTip = new QLabel(tr("Action"), this);
    ElaComboBox *actionCombo = new ElaComboBox(this);
    QLabel *triggerTypeTip = new QLabel(tr("Trigger"), this);
    ElaComboBox *triggerCombo = new ElaComboBox(this);

    QGridLayout *gLayout = new QGridLayout(this);
    gLayout->setColumnStretch(2, 1);
    gLayout->addWidget(actionTypeTip, 0, 0);
    gLayout->addWidget(actionCombo, 0, 1);
    gLayout->addWidget(triggerTypeTip, 0, 3);
    gLayout->addWidget(triggerCombo, 0, 4);
    gLayout->addWidget(shortcutTip, 1, 0);
    gLayout->addWidget(keyEdit, 1, 1, 1, 5);
    setSizeSettingKey("DialogSize/KeyActionEditDialog", QSize(400, 200));

    keyEdit->setObjectName(QStringLiteral("KeyEdit"));
    keyEdit->setFixedHeight(36);
    keyEdit->setFont(QFont(GlobalObjects::normalFont, 12));


    for (int i = 0; i < KeyAction::ActionType::ACT_NONE; ++i)
    {
        actionCombo->addItem(KeyAction::getActionName(KeyAction::ActionType(i)));
    }
    triggerCombo->addItem(tr("Press"), (int)KeyActionItem::ActionTrigger::KEY_PRESS);
    triggerCombo->addItem(tr("Release"), (int)KeyActionItem::ActionTrigger::KEY_RELEASE);

    QObject::connect(actionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KeyActionEditDialog::resetActionWidgets);
    QObject::connect(triggerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){
        this->triggerType = triggerCombo->currentData().toInt();
    });
    QObject::connect(keyEdit, &QKeySequenceEdit::keySequenceChanged, this, [=](const QKeySequence &keySeq){
        key = keySeq.toString();
    });

    if (item)
    {
        this->key = item->key;
        keyEdit->setKeySequence(QKeySequence(this->key));
        actionCombo->setCurrentIndex(item->action->actType);
        this->triggerType = item->triggerType;
        for (int i = 0; i < triggerCombo->count(); ++i)
        {
            if (triggerCombo->itemData(i).toInt() == triggerType)
            {
                triggerCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    if (!action)
    {
        resetActionWidgets(item ? item->action->actType : KeyAction::ActionType::ACT_PLAYPAUSE);
    }
}

KeyActionEditDialog::~KeyActionEditDialog()
{
    if (action)
    {
        delete action;
        action = nullptr;
    }
}

void KeyActionEditDialog::resetActionWidgets(int type)
{
    if (type == -1 || type == KeyAction::ActionType::ACT_NONE) return;
    if (action) delete action;
    qDeleteAll(actionParamWidgets);
    actionParamWidgets.clear();

    action = KeyAction::createAction(KeyAction::ActionType(type));
    QGridLayout *layout = static_cast<QGridLayout *>(this->layout());
    int curRow = layout->rowCount();
    bool useRefParam = refItem && refItem->action->actType == type;
    for (int i = 0; i < action->actParams.size(); ++i)
    {
        KeyAction::ActionParam &param = action->actParams[i];
        QLabel *tip = new QLabel(param.desc, this);
        if (useRefParam) param.val = refItem->action->actParams[i].val;
        switch(param.type)
        {
        case KeyAction::ParamType::PARAM_INT:
        {
            ElaSpinBox *spin = new ElaSpinBox(this);
            spin->setValue(param.val.toInt());
            layout->addWidget(tip, curRow, 0);
            layout->addWidget(spin, curRow, 1, 1, 5);
            actionParamWidgets << tip << spin;
            QObject::connect(spin, QOverload<int>::of(&ElaSpinBox::valueChanged), this, [i, this](int val){
                this->action->actParams[i].val = val;
            });
            break;
        }
        case KeyAction::PARAM_STR:
        {
            ElaLineEdit *edit = new ElaLineEdit(this);
            edit->setText(param.val.toString());
            layout->addWidget(tip, curRow, 0);
            layout->addWidget(edit, curRow, 1, 1, 5);
            actionParamWidgets << tip << edit;
            QObject::connect(edit, &QLineEdit::textChanged, this, [i, this](const QString &val){
                this->action->actParams[i].val = val.trimmed();
            });
            break;
        }
        }
        ++curRow;
    }

}

void KeyActionEditDialog::onAccept()
{
    if (key.isEmpty())
    {
        showMessage(tr("Key should not be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    if (KeyActionModel::instance()->hasKey(key))
    {
        if (!refItem || refItem->key != key)
        {
            showMessage(tr("Shortcut key conflicts with \"%1\"").arg(KeyActionModel::instance()->getActionItem(key)->action->getDesc()), NM_ERROR | NM_HIDE);
            return;
        }
    }
    for (int i = 0; i < action->actParams.size(); ++i)
    {
        KeyAction::ActionParam &param = action->actParams[i];
        if (param.val.isNull())
        {
            showMessage(tr("Action Param should not be empty"), NM_ERROR | NM_HIDE);
            return;
        }
    }
    CFramelessDialog::onAccept();
}
