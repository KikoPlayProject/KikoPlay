#include "ElaCalendarTitleModel.h"

ElaCalendarTitleModel::ElaCalendarTitleModel(QObject* parent)
    : QAbstractListModel{parent}
{
}

ElaCalendarTitleModel::~ElaCalendarTitleModel()
{
}

int ElaCalendarTitleModel::rowCount(const QModelIndex& parent) const
{
    return 7;
}

QVariant ElaCalendarTitleModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::UserRole)
    {
        switch (index.row())
        {
        case 0:
        {
            return tr("Sun");
        }
        case 1:
        {
            return tr("Mon");
        }
        case 2:
        {
            return tr("Tues");
        }
        case 3:
        {
            return tr("Wed");
        }
        case 4:
        {
            return tr("Thur");
        }
        case 5:
        {
            return tr("Fri");
        }
        case 6:
        {
            return tr("Sat");
        }
        }
    }
    return QVariant();
}
