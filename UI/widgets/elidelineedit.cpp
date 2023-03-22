#include "elidelineedit.h"
#include <QPushButton>
#include <QHBoxLayout>
#include "UI/inputdialog.h"

ElideLineEdit::ElideLineEdit(QWidget *parent) : QLineEdit(parent)
{
    setObjectName(QStringLiteral("ElideLineEdit"));
    QPushButton *editorBtn = new QPushButton("...", this);
    editorBtn->setObjectName(QStringLiteral("ElideLineEditBtn"));

    QHBoxLayout *editHLayout = new QHBoxLayout();
    editHLayout->addStretch(1);
    editHLayout->addWidget(editorBtn);
    editHLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(editHLayout);

    QObject::connect(editorBtn, &QPushButton::clicked, this, [=](){
        InputDialog input(tr("Edit"), "", this->text(), true, this, "DialogSize/ElideLineEditor");
        if(QDialog::Accepted == input.exec())
        {
            this->setText(input.text);
        }
    });
}
