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

#include "filemodel.h"

#include <QStandardPaths>

FileModel::FileModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles.insert(NameRole, QByteArray("name"));
    roles.insert(PathRole, QByteArray("path"));
    roles.insert(SizeRole, QByteArray("size"));
    return roles;
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return fileList.size();
}

void FileModel::searchFiles(QString path)
{
    QDir dir(path);
    QStringList accepted;
    accepted << "*.jpg"
             << "*.JPG"
             << "*.jpeg"
             << "*.JPEG"
             << "*.png"
             << "*.PNG"
             << "*.gif"
             << "*.GIF";
    const QFileInfoList &list = dir.entryInfoList(accepted, QDir::AllDirs | QDir::NoDot | QDir::NoSymLinks | QDir::Files, QDir::DirsFirst | QDir::Time);
    foreach (const QFileInfo &info, list) {
        if (info.fileName() == "..") {
            continue;
        }

        if (info.isDir()) {
            searchFiles(info.filePath());
        } else if (info.isFile()) {
            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            fileList.append(new FileInfo(info.fileName(), info.absoluteFilePath(), info.size()));
            endInsertRows();
        }
    }
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    FileInfo *fi = fileList.at(index.row());
    if (role == NameRole) {
        return QVariant::fromValue(fi->name);
    } else if (role == PathRole) {
        return QVariant::fromValue(fi->path);
    } else if (role == SizeRole) {
        return QVariant::fromValue(fi->size);
    }

    return QVariant();
}

QString FileModel::getSearchPath()
{
    QString res = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return res;
}

void FileModel::setSearchPath(QString path)
{
    Q_UNUSED(path);
    beginResetModel();
    qDeleteAll(fileList);
    fileList.clear();
    endResetModel();

    //Let's make this harbour compatible
    //searchFiles(path);
    searchFiles(getSearchPath());
}
