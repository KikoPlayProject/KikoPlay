#include "subtitleeditmodel.h"
#include <QRegularExpression>

SubtitleEditModel::SubtitleEditModel(bool enableTranslate, SubFile &subFile, QObject *parent) : QAbstractItemModel{parent},
    _subFile(subFile), _translate(enableTranslate)
{
    if (_translate)
    {
        _translateItems.resize(_subFile.items.size());
    }
}

void SubtitleEditModel::setEnableTranslate(bool on)
{
    if (on == _translate) return;

    beginResetModel();
    if (on)
    {
        _translate = true;
        _translateItems.resize(_subFile.items.size());

    }
    else
    {
        _translate = false;
    }
    endResetModel();
}

SubFile SubtitleEditModel::getTranslatedSubFile(bool onlyTranslated, bool translatedFirst)
{
    if (!_translate || _subFile.items.size() != _translateItems.size()) return SubFile();

    SubFile tSubFile(_subFile);
    if (onlyTranslated)
    {
        for (int i = 0; i < _subFile.items.size(); ++i)
        {
            tSubFile.items[i].text = _translateItems[i];
        }
    }
    else
    {
        for (int i = 0; i < _subFile.items.size(); ++i)
        {
            if (translatedFirst)
            {
                tSubFile.items[i].text = _translateItems[i] + "\n" + tSubFile.items[i].text;
            }
            else
            {
                tSubFile.items[i].text += "\n" + _translateItems[i];
            }
        }
    }
    return tSubFile;
}

void SubtitleEditModel::removeItems(QVector<int> rows)
{
    std::sort(rows.rbegin(), rows.rend());
    for (int row : rows)
    {
        beginRemoveRows(QModelIndex(), row, row);
        _subFile.items.removeAt(row);
        if (_translate) _translateItems.removeAt(row);
        endRemoveRows();
    }
}

QVariant SubtitleEditModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const SubItem &item = _subFile.items[index.row()];
    Columns col = static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::EditRole:
    {
        switch (col)
        {
        case Columns::START:
        {
            return item.encodeTime(item.startTime);
        }
        case Columns::END:
        {
            return item.encodeTime(item.endTime);
        }
        case Columns::TEXT:
        {
            return item.text;
        }
        case Columns::TEXT_TRANSLATED:
            if (_translate) return _translateItems[index.row()];
            break;
        }
        break;
    }
    default:
        return QVariant();
    }
    return QVariant();
}

QVariant SubtitleEditModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static const QStringList headers{ tr("Start"), tr("End"), tr("Text"), tr("​Translated Text​")  };
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section < headers.size()) return headers.at(section);
    }
    if (role == Qt::DisplayRole && orientation == Qt::Vertical)
    {
        return section;
    }
    return QVariant();
}

bool SubtitleEditModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();
    Columns col = static_cast<Columns>(index.column());
    SubItem &item = _subFile.items[index.row()];
    static const QRegularExpression timeReg = QRegularExpression("\\d+:\\d+:\\d+\\.\\d+");
    switch (col)
    {
    case Columns::START:
    {
        auto match = timeReg.match(value.toString());
        if (match.hasMatch())
        {
            item.startTime = SubItem::timeStringToMs(match.captured());
        }
        break;
    }
    case Columns::END:
    {
        auto match = timeReg.match(value.toString());
        if (match.hasMatch())
        {
            item.endTime = SubItem::timeStringToMs(match.captured());
        }
        break;
    }
    case Columns::TEXT:
        item.text = value.toString();
        break;
    case Columns::TEXT_TRANSLATED:
        _translateItems[row] = value.toString();
        break;
    default:
        return false;
    }
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags SubtitleEditModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    return  Qt::ItemIsEditable | defaultFlags;
}
