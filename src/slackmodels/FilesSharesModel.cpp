#include "FilesSharesModel.h"
#include "UsersModel.h"
#include "MessagesModel.h"

FilesSharesModel::FilesSharesModel(QAbstractListModel *parent,
                                         UsersModel *usersModel,
                                         const QString &teamId) :
    QAbstractListModel(parent), m_teamId(teamId) {}

void FilesSharesModel::addFileShares(const fsList &fshares, int total, int page, int pages)
{
    qDebug() << "Adding" << fshares.count() << "file shares";
    if (m_total != total) {
        m_total = total;
    }
    if (m_totalPages != pages) {
        m_totalPages = pages;
    }
    QMutexLocker locker(&m_mutex);
    beginInsertRows(QModelIndex(), m_fileShares.count(), m_fileShares.count() + fshares.count() - 1);
    m_fileShares << fshares;
    endInsertRows();
    m_fetched += fshares.count();
    m_lastPageFetched = page;
    m_pagesRetrieved.insert(page);
}

void FilesSharesModel::retreiveFilesFor(const QString &channel, const QString &user)
{
    m_channelId = channel;
    m_userId = user;
    if (m_fetched > 0) {
        beginResetModel();
        m_fileShares.clear();
        m_total = 0;
        m_pagesRetrieved.clear();
        m_fetched = 0;
        int m_lastPageFetched = -1;
        int m_totalPages = 0 ;
        endResetModel();
    }
    emit fetchMoreData(1, m_channelId, m_userId);
}

int FilesSharesModel::rowCount(const QModelIndex &parent) const
{
    return m_fetched;
}

bool FilesSharesModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qDebug() << "called can fetch more" << parent << m_fetched << m_total;
    return m_fetched < m_total;
}

void FilesSharesModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    emit fetchMoreData(m_lastPageFetched+1, m_channelId, m_userId);
}


QVariant FilesSharesModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row >= m_fileShares.count() || row < 0) {
        qWarning() << "invalid row" << row;
        return QVariant();
    }

    switch (role) {
    case FileShareObject:
        return QVariant::fromValue(m_fileShares.at(row));
    default:
        qWarning() << "Invalid role" << role;
        return QVariant();
    }
}


QHash<int, QByteArray> FilesSharesModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[FileShareObject] = "FileShareObject";
    return names;
}
