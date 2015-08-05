#ifndef RESTRICTIVESQLTABLEMODEL_H
#define RESTRICTIVESQLTABLEMODEL_H

#include <QObject>
#include <QSqlTableModel>

class RestrictiveSqlTableModel : public QSqlTableModel
{
    Q_OBJECT
public:
    RestrictiveSqlTableModel(QObject * parent = 0, QSqlDatabase db = QSqlDatabase())
    : QSqlTableModel(parent,db) {;}

protected:
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
};

#endif // RESTRICTIVESQLTABLEMODEL_H
