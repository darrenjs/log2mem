#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <QAbstractTableModel>
#include <QLinkedList>


class AppMan;

struct TableModelItem
{
    QString rowid;
    QString type;
    QString label;
    QString value;
};

class TableModel : public QAbstractTableModel
{
    Q_OBJECT
public:


    enum Column {MemmapID, Typename, Label, Value};

    static const int NumColumns = 4;

    TableModel(AppMan*);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;

    virtual QVariant headerData(int section,
                                Qt::Orientation orientation,
                                int role) const;

    void refresh();
    void rebuild_table();

    void insert();
  private:
    AppMan* m_appman;
    QList<TableModelItem> m_data;
};

#endif // TABLEMODEL_H
