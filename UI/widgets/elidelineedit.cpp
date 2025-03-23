#include "elidelineedit.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidgetAction>
#include "UI/inputdialog.h"
#include "globalobjects.h"

ElideLineEdit::ElideLineEdit(QWidget *parent) : KLineEdit(parent)
{
    setObjectName(QStringLiteral("ElideLineEdit"));

    QPushButton *editorBtn = new QPushButton(this);
    editorBtn->setObjectName(QStringLiteral("ElideLineEditBtn"));
    GlobalObjects::iconfont->setPointSize(12);
    editorBtn->setFont(*GlobalObjects::iconfont);
    editorBtn->setText(QChar(0xe8ac));
    editorBtn->setCursor(Qt::ArrowCursor);
    editorBtn->setFocusPolicy(Qt::NoFocus);
    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(editorBtn);
    addAction(optionsAction, QLineEdit::TrailingPosition);

    QObject::connect(editorBtn, &QPushButton::clicked, this, [=](){
        InputDialog input(tr("Edit"), "", this->text(), true, this, "DialogSize/ElideLineEditor");
        if(QDialog::Accepted == input.exec())
        {
            this->setText(input.text);
        }
    });
}
