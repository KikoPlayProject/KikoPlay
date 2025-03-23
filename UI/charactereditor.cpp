#include "charactereditor.h"
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include "MediaLibrary/animeinfo.h"
#include "Common/notifier.h"
#include "UI/ela/ElaLineEdit.h"


CharacterEditor::CharacterEditor(Anime *anime, const Character *crt,  QWidget *parent) :
    CFramelessDialog(crt? tr("Edit Character") : tr("Add Character"), parent, true, true, false),
    curAnime(anime), curCrt(crt)
{
    QLabel *nameTip = new QLabel(tr("Character Name"), this);
    QLabel *linkTip = new QLabel(tr("Link"), this);
    QLabel *actorTip = new QLabel(tr("Actor"), this);
    QLineEdit *nameEdit = new ElaLineEdit(this);
    QLineEdit *linkEdit = new ElaLineEdit(this);
    QLineEdit *actorEdit = new ElaLineEdit(this);

    QObject::connect(nameEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        name = text;
    });
    QObject::connect(linkEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        link = text;
    });
    QObject::connect(actorEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        actor = text;
    });

    if (crt)
    {
        nameEdit->setText(crt->name);
        linkEdit->setText(crt->link);
        actorEdit->setText(crt->actor);
    }

    QGridLayout *shortcutGLayout=new QGridLayout(this);
    shortcutGLayout->addWidget(nameTip,0,0);
    shortcutGLayout->addWidget(nameEdit,0,1);
    shortcutGLayout->addWidget(linkTip,1,0);
    shortcutGLayout->addWidget(linkEdit,1,1);
    shortcutGLayout->addWidget(actorTip,2,0);
    shortcutGLayout->addWidget(actorEdit,2,1);
    shortcutGLayout->setColumnStretch(1,1);
    setSizeSettingKey("DialogSize/CharacterEdit", QSize(400, 120));
}

void CharacterEditor::onAccept()
{
    if(name.trimmed().isEmpty())
    {
        showMessage(tr("Character Name should not be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    name = name.trimmed();
    QString oldName = curCrt? curCrt->name:"";
    if(curAnime && name != oldName)
    {
        for(const Character &crt : curAnime->crList())
        {
            if(crt.name != oldName && crt.name == name)
            {
                 showMessage(tr("Character \"%1\" already exists").arg(name), NM_ERROR | NM_HIDE);
                 return;
            }
        }
    }
    link = link.trimmed();
    actor = actor.trimmed();
    CFramelessDialog::onAccept();
}
