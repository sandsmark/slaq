#include "searchmessagesmodel.h"

SearchMessagesModel::SearchMessagesModel(QObject *parent,
                                         UsersModel *usersModel,
                                         const QString &channelId) :
    MessageListModel(parent, usersModel, channelId) {}

QList<int> SearchMessagesModel::searchPagesRetrieved() const
{
    return m_searchPagesRetrieved;
}

void SearchMessagesModel::addSearchMessages(const QJsonArray &messages, int page)
{
    qDebug() << "Adding" << messages.count() << "search messages";
    QMutexLocker locker(&m_modelMutex);
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count() + messages.count() - 1);
    int _index = page * 100 - 1;

    for (const QJsonValue &messageData : messages) {
        const QJsonObject messageObject = messageData.toObject();
        if (messageObject.value(QStringLiteral("subtype")).toString() == "file_comment") {
            continue; //TODO: not yet supported
        }
        Message* message = new Message;
        //qDebug() << "message obj" << messageObject;
        message->setData(messageObject);
        if (message->user_id.isEmpty()) {
            qWarning() << "user id is empty" << messageObject;
        }
        message->user = m_usersModel->user(message->user_id);

        preprocessFormatting(nullptr, message);
        m_messages.append(message);
        _index--;
    }

    endInsertRows();
    m_lastPageFetched = page;
}

int SearchMessagesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_fetched;
}

void SearchMessagesModel::searchPageRetrieved(int page, int items)
{
    if (!m_searchPagesRetrieved.contains(page)) {
        m_searchPagesRetrieved.append(page);
        m_fetched += items;
    }
}

QString SearchMessagesModel::searchQuery() const
{
    return m_searchQuery;
}

void SearchMessagesModel::setSearchQuery(const QString &searchQuery)
{
    if (searchQuery != m_searchQuery) {
        m_searchPagesRetrieved.clear();
        m_searchQuery = searchQuery;
        m_fetched = 0;
        m_lastPageFetched = -1;
        m_total = 0;
    }
}

void SearchMessagesModel::preallocateTotal(int total)
{
    m_total = total;
}

bool SearchMessagesModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qDebug() << "called can fetch more" << parent << m_fetched << m_total;
    return m_fetched < m_total;
}

void SearchMessagesModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    qDebug() << "called fetch more" << parent;
    emit fetchMoreData(m_searchQuery, m_lastPageFetched+1);
}
