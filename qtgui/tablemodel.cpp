#include "tablemodel.h"
#include "appman.h"

#include "log2mem/log2mem_dev.h"

#include <QStyleOptionComboBox>



//----------------------------------------------------------------------
TableModel::TableModel(AppMan* appman)
  : QAbstractTableModel(NULL),
    m_appman(appman)
{
  refresh();

  this->setHeaderData(0, Qt::Horizontal, tr("ID"));
  this->setHeaderData(1, Qt::Horizontal, tr("First name"));
  this->setHeaderData(2, Qt::Horizontal, tr("Last name"));
}

//----------------------------------------------------------------------
QVariant TableModel::headerData(int section,
                                Qt::Orientation orientation,
                                int role) const
{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal)
  {
    switch (section)
    {
      case MemmapID: return tr("id");
      case Typename: return tr("type");
      case Label: return tr("label");
      case Value: return tr("value");
      default: Q_ASSERT(false);
    }
  }

  return section + 1;
}

//----------------------------------------------------------------------
QVariant TableModel::data(const QModelIndex &index,
                          int role ) const
{
  QString MaxZipcode = "xxxxx";
   if (!index.isValid() ||
       index.row() < 0 || index.row() >= m_data.count() ||
       index.column() < 0 || index.column() >= NumColumns)
     return QVariant();

   const TableModelItem &item = m_data.at(index.row());

  if (role == Qt::SizeHintRole)
  {
    QStyleOptionComboBox option;

    switch (index.column())
    {
      case MemmapID:
      {
        option.currentText = MaxZipcode;
        const QString header = headerData(MemmapID,
                                          Qt::Horizontal,
                                          Qt::DisplayRole).toString();
        if (header.length() > option.currentText.length())
          option.currentText = header;
        break;
      }
      case Typename:
      {
        option.currentText = item.type;
        break;
      }
      case Label: option.currentText = item.label; break;
      case Value: option.currentText = item.value; break;
      default: Q_ASSERT(false);
    }

    QFontMetrics fontMetrics(data(index, Qt::FontRole)
                             .value<QFont>());
    option.fontMetrics = fontMetrics;
    QSize size(fontMetrics.width(option.currentText),
               fontMetrics.height());

    return m_appman->style()->sizeFromContents(QStyle::CT_ComboBox,
                                           &option, size);
  }

  if (role == Qt::DisplayRole || role == Qt::EditRole)
  {
    switch (index.column())
    {
      case MemmapID : return item.rowid;
      case Typename : return item.type;
      case Label    : return item.label;
      case Value    : return item.value;
      default: Q_ASSERT(false);
    }
  }

  // for all unhandled cases
  return QVariant();
}

//----------------------------------------------------------------------
void TableModel::refresh()
{
  TableModelItem item;
  item.rowid = "1";
  item.type  = "1";
  item.label = "enable";
  item.value = "2.32";

  int newrows_firstindex =  m_data.count();
  int newrows_lastindex  =  m_data.count();

  this->beginInsertRows(QModelIndex(),  newrows_firstindex, newrows_lastindex);
  m_data.push_back( item );
  this->endInsertRows();

}

//----------------------------------------------------------------------
int TableModel::rowCount(const QModelIndex & index) const
{
  return index.isValid() ? 0 : m_data.count();
}

//----------------------------------------------------------------------
int TableModel::columnCount(const QModelIndex & index) const
{
  return 4;
}

//----------------------------------------------------------------------
void TableModel::rebuild_table()
{
  /*
    NOTE: not using the beginResetModel & endResetModel methods, because it
    will cause all table sections, and scroll positions, to be lost.

    this->beginResetModel();
    this->endResetModel();
  */
  struct log2mem_handle * handle = 0;

  const char* filename="/tmp/log2mem.dat";
  handle = log2mem_attach(filename);


  unsigned int num_vars = handle->header->config.num_vars;


/*
  get a variable: index; type;  ... index is the key item.



  1.double.=1.1

  does [i] exist? NO:   --> insert
                  YES:  --> changed?



*/

//  this->beginInsertRows(QModelIndex(),  newrows_firstindex, newrows_lastindex);
//  m_data.push_back( item );
//  this->endInsertRows();

  // TODO: scan the memory map, looking for the current contents.

  // TODO: merge in the changes, which causes either row inserts, row
  // deletions, or row changes.
}
