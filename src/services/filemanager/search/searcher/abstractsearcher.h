/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     liuzhangjian<liuzhangjian@uniontech.com>
 *
 * Maintainer: liuzhangjian<liuzhangjian@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ABSTRACTSEARCHER_H
#define ABSTRACTSEARCHER_H

#include "dfm_filemanager_service_global.h"

#include <QObject>
#include <QUrl>

DSB_FM_BEGIN_NAMESPACE

class AbstractSearcher : public QObject
{
    Q_OBJECT
public:
    enum Status {
        kReady,
        kRuning,
        kCompleted,
        kTerminated
    };

    explicit AbstractSearcher(const QUrl &url, const QString &key, QObject *parent = nullptr);
    virtual bool search() = 0;
    virtual void stop() = 0;
    virtual bool hasItem() const = 0;
    virtual QList<QUrl> takeAll() = 0;
signals:
    void unearthed(AbstractSearcher *searcher);

protected:
    QUrl searchUrl;
    QString keyword;
};

DSB_FM_END_NAMESPACE

#endif   // ABSTRACTSEARCHER_H
