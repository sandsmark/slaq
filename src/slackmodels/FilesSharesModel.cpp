#include "FilesSharesModel.h"
#include "UsersModel.h"
#include "MessagesModel.h"

FilesSharesModel::FilesSharesModel(QObject *parent,
                                         const QString &teamId) :
    QAbstractListModel(parent), m_teamId(teamId)
{
}

void FilesSharesModel::addFileShares(const QList<FileShare *> &fshares, int total, int page, int pages)
{
    qDebug() << "Adding" << fshares.count() << "file shares";
    if (m_total != total) {
        m_total = total;
    }
    if (m_totalPages != pages) {
        m_totalPages = pages;
    }

    int _initialCount = m_filesIds.size();

    QMutexLocker locker(&m_mutex);
    for (FileShare* fs : fshares) {
        int _row = m_filesIds.value(fs->m_id, -1);
        if (_row != -1) {
            m_fileShares.replace(_row, fs);
            locker.unlock();
            QModelIndex index = QAbstractListModel::index(_row, 0,  QModelIndex());
            emit dataChanged(index, index);
            locker.relock();
        } else {
            m_fileShares.append(fs);
            m_filesIds.insert(fs->m_id, m_fileShares.size() - 1);
        }
    }

    beginInsertRows(QModelIndex(), _initialCount, m_filesIds.size() - _initialCount - 1);
    endInsertRows();
    m_fetched += (m_filesIds.size() - _initialCount);
    m_lastPageFetched = page;
    m_pagesRetrieved.insert(page);
}

void FilesSharesModel::retreiveFilesFor(const QString &channel, const QString &user)
{
    m_channelId = channel;
    m_userId = user;
    if (m_fetched > 0) {
        beginResetModel();
        qDeleteAll(m_fileShares);
        m_fileShares.clear();
        m_filesIds.clear();
        m_total = 0;
        m_pagesRetrieved.clear();
        m_fetched = 0;
        endResetModel();
    }
    emit fetchMoreData(1, m_channelId, m_userId);
}

void FilesSharesModel::deleteFile(const QString &fileId)
{
    QMutexLocker locker(&m_mutex);
    int _index = m_filesIds.value(fileId, -1);
    if (_index >= 0 && _index < m_fileShares.size()) {
        FileShare* _fshare = m_fileShares.at(_index);
        beginRemoveRows(QModelIndex(), _index, _index);
        m_fileShares.removeAt(_index);
        m_filesIds.remove(fileId);
        endRemoveRows();
        delete _fshare;
    }
}

int FilesSharesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

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
    Q_UNUSED(parent);

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
