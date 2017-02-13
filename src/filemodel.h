/*

Hanghish
Copyright (C) 2015 Daniele Rogora

This file is part of Hangish.

Hangish is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Hangish is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>

*/


#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QAbstractListModel>

class FileInfo {
public:
    QString name;
    QString path;
    int size;

    FileInfo(QString pname, QString ppath, int psize) {
        path = ppath;
        name = pname;
        size = psize;
    }
};

class FileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchPath READ getSearchPath WRITE setSearchPath FINAL)

public:
    enum FileModelRoles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SizeRole
    };
    explicit FileModel(QObject *parent = 0);
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QString getSearchPath();
    void setSearchPath(QString path);

private:
    QList<FileInfo *> fileList;
    void searchFiles(QString path);

signals:

public slots:

protected:
    QHash<int, QByteArray> roleNames() const;
};

#endif // FILEMODEL_H
