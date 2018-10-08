#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QMutex>
#include <QSet>

class UsersModel;
class FileShare;

typedef QList<FileShare *> fsList;

class FilesSharesModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum FileSharesFields {
        FileShareObject
    };
    Q_ENUM(FileSharesFields)

    FilesSharesModel(QAbstractListModel *parent, UsersModel *usersModel, const QString& teamId);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void addFileShares(const fsList &fshares, int total, int page, int pages);

    /**
     * @brief retreiveFilesFor - resets model and fetch data for user or channel. By defaul it retreive all the data
     * @param channel
     * @param user
     */
    void retreiveFilesFor(const QString& channel, const QString& user);
protected:
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

signals:
    void fetchMoreData(int page, const QString& channel, const QString& user);

private:
    QString m_teamId;
    QString m_userId;
    QString m_channelId;
    QSet<int> m_pagesRetrieved;
    fsList m_fileShares;
    int m_fetched { 0 };
    int m_lastPageFetched { -1 };
    int m_total { 0 };
    int m_totalPages { 0 };
    QMutex m_mutex;
};
