#ifndef SEARCHMESSAGESMODEL_H
#define SEARCHMESSAGESMODEL_H

#include <QObject>
#include "MessagesModel.h"

class SearchMessagesModel : public MessageListModel
{
    Q_OBJECT
public:
    SearchMessagesModel(QObject *parent, UsersModel *usersModel, const QString& channelId);

    QList<int> searchPagesRetrieved() const;
    void searchPageRetrieved(int page, int items);
    QString searchQuery() const;
    void setSearchQuery(const QString &searchQuery);
    void preallocateTotal(int total);
    void addSearchMessages(const QJsonArray &messages, int page);

    int rowCount(const QModelIndex &parent) const override;
protected:
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

signals:
    void fetchMoreData(const QString& query, int page);

private:
    QString m_searchQuery;
    QList<int> m_searchPagesRetrieved;
    int m_fetched { 0 };
    int m_lastPageFetched { -1 };
    int m_total { 0 };

};

#endif // SEARCHMESSAGESMODEL_H
