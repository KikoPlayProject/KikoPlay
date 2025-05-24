#include "scriptmodel.h"
#include "globalobjects.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include "UI/stylemanager.h"
#define ScriptTypeRole Qt::UserRole+1
#define ScriptHasSettingRole Qt::UserRole+2
#define ScriptHasMenuRole Qt::UserRole+3

ScriptModel::ScriptModel(QObject *parent) : QAbstractItemModel(parent)
{
    refresh();
    QObject::connect(GlobalObjects::scriptManager, &ScriptManager::scriptChanged, this, &ScriptModel::refresh);
}

QVariant ScriptModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= scriptList.size()) return QVariant();
    ScriptModel::Columns col = static_cast<Columns>(index.column());
    const ScriptInfo &script = scriptList.at(index.row());
    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
        case Columns::TYPE:
            if(script.type != ScriptType::UNKNOWN_STYPE)
                return scriptTypes[script.type];
            break;
        case Columns::ID:
            return script.id;
        case Columns::NAME:
            return script.name;
        case Columns::VERSION:
            return script.version;
        case Columns::DESC:
            return script.desc;
        default:
            break;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        switch (col)
        {
        case Columns::ID:
            return script.id;
        case Columns::NAME:
            return script.name;
        case Columns::VERSION:
            return script.version;
        case Columns::DESC:
            return script.desc;
        default:
            break;
        }
    }
    else if (role == ScriptTypeRole)
    {
        return script.type;
    }
    else if (role == ScriptHasSettingRole)
    {
        return script.hasSetting;
    }
    else if (role == ScriptHasMenuRole)
    {
        return script.hasMenu;
    }
    return QVariant();
}

QVariant ScriptModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static QStringList headers({ tr("Type"), tr("Id"), tr("Name"), tr("Version"), tr("Operate"), tr("Description") });
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size()) return headers.at(section);
    }
    return QVariant();
}

void ScriptModel::refresh()
{
    beginResetModel();
    scriptList.clear();
    for(int i=0;i<ScriptType::UNKNOWN_STYPE;++i)
    {
        auto type = ScriptType(i);
        for(const auto &s: GlobalObjects::scriptManager->scripts(type))
        {
            scriptList.append({type, s->id(), s->name(), s->version(), s->desc(), s->getValue("path"), !s->settings().empty(), !s->getScriptMenuItems().empty()});
        }
    }
    endResetModel();
}

void ScriptProxyModel::setType(int type)
{
    ctype = type;
    invalidateFilter();
}

bool ScriptProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if(ctype==-1) return true;
    QModelIndex index = sourceModel()->index(source_row, static_cast<int>(ScriptModel::Columns::ID), source_parent);
    return ctype == index.data(ScriptTypeRole).value<ScriptType>();
}

void ScriptItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KTreeviewItemDelegate::paint(painter, option, index);
    if (!index.isValid()) return;
    painter->save();
    if (index.column() == static_cast<int>(ScriptModel::Columns::OPERATE))
    {
        auto getIcon = [](int size, int pixelSize, QChar c, QColor penColor){
            float pxR = GlobalObjects::context()->devicePixelRatioF;
            QPixmap pix(size * pxR, size * pxR);
            pix.setDevicePixelRatio(pxR);
            pix.fill(Qt::transparent);
            QPainter painter;
            painter.begin(&pix);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            GlobalObjects::iconfont->setPixelSize(pixelSize);
            painter.setPen(penColor);
            painter.setFont(*GlobalObjects::iconfont);
            painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, c);
            painter.end();
            return pix;
        };

        const int pixelSize = iconSize;

        QPoint startPos = getIconStartPos(option, index);
        bool hasSetting = index.data(ScriptHasSettingRole).toBool();
        bool hasMenu = index.data(ScriptHasMenuRole).toBool();
        const QColor hoverColor = StyleManager::getStyleManager()->enableThemeColor() ? StyleManager::getStyleManager()->curThemeColor() : QColor{96, 208, 252};

        if (hasSetting)
        {
            QColor penColor = QRect(startPos, QSize(iconSize, iconSize)).contains(mousePos) ? hoverColor : Qt::white;
            painter->drawPixmap(startPos, getIcon(iconSize, pixelSize, QChar(0xe607), penColor));
            startPos.rx() += iconSize + space;
        }
        if (hasMenu)
        {
            QColor penColor = QRect(startPos, QSize(iconSize, iconSize)).contains(mousePos) ? hoverColor : Qt::white;
            painter->drawPixmap(startPos, getIcon(iconSize, pixelSize, QChar(0xe644), penColor));
            startPos.rx() += iconSize + space;
        }
    }
    painter->restore();
}

QSize ScriptItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == static_cast<int>(ScriptModel::Columns::OPERATE))
    {
        int totalWidth = iconSize + space * 2;
        if (index.data(ScriptHasSettingRole).toBool()) totalWidth += space + iconSize;
        if (index.data(ScriptHasMenuRole).toBool()) totalWidth += space + iconSize;

        return QSize(totalWidth, iconSize + space * 2);

    }
    return KTreeviewItemDelegate::sizeHint(option, index);
}

bool ScriptItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QMouseEvent *pEvent = static_cast<QMouseEvent *> (event);
    switch (event->type())
    {
    case QEvent::MouseMove:
    {
        mousePos = pEvent->pos();
        break;
    }
    case QEvent::MouseButtonPress:
    {
        if (pEvent->button() == Qt::LeftButton && !(pEvent->modifiers()&Qt::ControlModifier) && !(pEvent->modifiers()&Qt::ShiftModifier))
        {
            QRect settingRect(getIconRect(0, option, index)), menuRect(getIconRect(1, option, index));
            if (settingRect.isValid() && settingRect.contains(pEvent->pos()))
                emit settingClicked(index);
            else if (menuRect.isValid() && menuRect.contains(pEvent->pos()))
                emit menuClicked(index);
        }
        break;
    }
    default:
        break;
    }
    return KTreeviewItemDelegate::editorEvent(event,model,option,index);
}

QPoint ScriptItemDelegate::getIconStartPos(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int totalWidth = 0;
    bool hasSetting = index.data(ScriptHasSettingRole).toBool();
    bool hasMenu = index.data(ScriptHasMenuRole).toBool();
    if (hasSetting) totalWidth += space + iconSize;
    if (hasMenu) totalWidth += space + iconSize;

    int x = (option.rect.width() - totalWidth) / 2 + option.rect.x();
    int y = (option.rect.height() - iconSize) / 2 + option.rect.y();
    return QPoint(x, y);
}

QRect ScriptItemDelegate::getIconRect(int type, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QPoint startPos = getIconStartPos(option, index);
    bool hasSetting = index.data(ScriptHasSettingRole).toBool();
    bool hasMenu = index.data(ScriptHasMenuRole).toBool();

    if (type == 0)  // setting
    {
        return hasSetting ? QRect(startPos, QSize(iconSize, iconSize)) : QRect();
    }
    else if (type == 1)  // menu
    {
        QPoint move{hasSetting ? iconSize + space : 0, 0};
        return hasMenu ? QRect(startPos + move, QSize(iconSize, iconSize)) : QRect();
    }
    return QRect();
}
