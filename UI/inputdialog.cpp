#include "inputdialog.h"
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include "Common/notifier.h"

InputDialog::InputDialog(const QString &title, const QString &tip, const QString &text, bool canEmpty, QWidget *parent)
    : CFramelessDialog (title,parent,true)
{
   QLabel *tipLabel=new QLabel(tip,this);
   edit=new QPlainTextEdit(text,this);
   edit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
   QVBoxLayout *inputVLayout=new QVBoxLayout(this);
   inputVLayout->addWidget(tipLabel);
   inputVLayout->addWidget(edit);
   this->canEmpty=canEmpty;
}

void InputDialog::onAccept()
{
    text=edit->toPlainText().trimmed();
    if(!canEmpty && text.isEmpty())
    {
        showMessage(tr("Can't be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    CFramelessDialog::onAccept();
}
