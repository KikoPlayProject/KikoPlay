#include "ElaCheckBox.h"
#include "globalobjects.h"

#include "DeveloperComponents/ElaCheckBoxStyle.h"
ElaCheckBox::ElaCheckBox(QWidget* parent)
    : QCheckBox(parent)
{
    _pBorderRadius = 3;
    setMouseTracking(true);
    setObjectName("ElaCheckBox");
    setStyle(new ElaCheckBoxStyle(style()));
    QFont widgetFont(GlobalObjects::normalFont);
    widgetFont.setPixelSize(qRound(GlobalObjects::fontSize(15)));
    setFont(widgetFont);
}

ElaCheckBox::ElaCheckBox(const QString& text, QWidget* parent)
    : ElaCheckBox(parent)
{
    setText(text);
}

ElaCheckBox::~ElaCheckBox()
{
}
