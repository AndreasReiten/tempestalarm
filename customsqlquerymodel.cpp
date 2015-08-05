#include "customsqlquerymodel.h"

#include <QBrush>
#include <QDebug>

QVariant CustomSqlQueryModel::data ( const QModelIndex & index, int role ) const
{
    if(role == Qt::BackgroundRole)
    {
        const QVariant value(data(index.sibling(index.row(),4),Qt::DisplayRole));

        int R = 510 -(255 + value.toInt()*2.5);
        if (R > 255) R = 255;
        if (R < 0) R = 0;

        int G = 255 + value.toInt()*2.5;
        if (G > 255) G = 255;
        if (G < 0) G = 0;

        int B = 255 - fabs(value.toInt()*2.5);
        if (B > 255) B = 255;
        if (B < 0) B = 0;

        QColor color(R,G,B);

        QBrush brush(color);

        return brush;
    }
    else if (role == Qt::TextColorRole)
    {
        return QColor(0,0,0);
    }
    else if(role == Qt::ToolTipRole)
    {
        return QString("");
    }

    return QSqlQueryModel::data(index,role);
}

