/***************************************************************************
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "xml/xml.hpp"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <locale>
#ifdef Q_OS_MAC
#include <xlocale.h>
#endif


template<typename AssetType>
AbstractAssetsRepository<AssetType>::AbstractAssetsRepository()
{
}


template<typename AssetType>
void AbstractAssetsRepository<AssetType>::init()
{
        // Warning: Mlt::Factory::init() resets the locale to the default system value, make sure we keep correct locale
#ifndef Q_OS_MAC
    setlocale(LC_NUMERIC, nullptr);
#else
    setlocale(LC_NUMERIC_MASK, nullptr);
#endif

    // Parse effects blacklist
    parseBlackList(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("blacklisted_effects.txt")));

    // Retrieve the list of MLT's available assets.
    QScopedPointer<Mlt::Properties> assets(retrieveListFromMlt());
    int max = assets->count();
    for (int i = 0; i < max; ++i) {
        Info info;
        QString name = assets->get_name(i);
        if (!m_blacklist.contains(name) && parseInfoFromMlt(name, info)) {
            m_assets[name] = info;
        } else {
            if (m_blacklist.contains(name)) {
                qDebug() << name << "is blacklisted";
            } else {
                qDebug() << "WARNING : Fails to parse "<<name;
            }
        }
    }

    // We now parse custom effect xml
    // Set the directories to look into for effects.
    QStringList asset_dirs = assetDirs();

    for (const auto& dir : asset_dirs) {
        QDir current_dir(dir);
        QStringList filter;
        filter << QStringLiteral("*.xml");
        QStringList fileList = current_dir.entryList(filter, QDir::Files);
        for (const auto& file : fileList) {
            QString path = current_dir.absoluteFilePath(file);
            parseCustomAssetFile(path);
        }
    }

}

template<typename AssetType>
void AbstractAssetsRepository<AssetType>::parseBlackList(const QString& path)
{
    QFile blacklist_file(path);
    if (blacklist_file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&blacklist_file);
        QString line;
        while (stream.readLineInto(&line)) {
            line = line.simplified();
            if (!line.isEmpty() && !line.startsWith('#')) {
                m_blacklist.insert(line);
            }
        }
        blacklist_file.close();
    }
}


template<typename AssetType>
bool AbstractAssetsRepository<AssetType>::exists(const QString& assetId) const
{
    return m_assets.count(assetId) > 0;
}

template<typename AssetType>
QVector<QPair<QString,QString> > AbstractAssetsRepository<AssetType>::getNames() const
{
    QVector<QPair<QString,QString> > res;
    res.reserve((int)m_assets.size());
    for (const auto& asset : m_assets) {
        res.push_back({asset.first, asset.second.name});
    }
    std::sort(res.begin(), res.end(), [](const decltype(res.front())& a, const decltype(res.front())& b){
            return a.second < b.second;
        });
    return res;
}

template<typename AssetType>
AssetType AbstractAssetsRepository<AssetType>::getType(const QString& assetId) const
{
    Q_ASSERT(m_assets.count(assetId) > 0);
    return m_assets.at(assetId).type;
}

template<typename AssetType>
QString AbstractAssetsRepository<AssetType>::getName(const QString& assetId) const
{
    Q_ASSERT(m_assets.count(assetId) > 0);
    return m_assets.at(assetId).name;
}

template<typename AssetType>
QString AbstractAssetsRepository<AssetType>::getDescription(const QString& assetId) const
{
    Q_ASSERT(m_assets.count(assetId) > 0);
    return m_assets.at(assetId).description;
}

template<typename AssetType>
bool AbstractAssetsRepository<AssetType>::isFavorite(const QString& /*assetId*/) const
{
    //TODO
    return true;
}
