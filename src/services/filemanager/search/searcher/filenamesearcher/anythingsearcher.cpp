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
#include "anythingsearcher.h"
#include "search/utils/regularexpression.h"

#include "dfm-base/base/urlroute.h"
#include "dfm-base/dbusservice/dbus_interface/anything_interface.h"

#include <QDBusReply>
#include <QDebug>

namespace {
static int kEmitInterval = 50;   // 推送时间间隔（ms）
static qint32 kMaxCount = 100;   // 最大搜索结果数量
static qint64 kMaxTime = 500;   // 最大搜索时间（ms）
}

DFMBASE_USE_NAMESPACE
DSB_FM_BEGIN_NAMESPACE

AnythingSearcher::AnythingSearcher(const QUrl &url, const QString &keyword, bool dataFlag, QObject *parent)
    : AbstractSearcher(url, RegularExpression::checkWildcardAndToRegularExpression(keyword), parent),
      isPrependData(dataFlag)
{
    anythingInterface = new ComDeepinAnythingInterface("com.deepin.anything",
                                                       "/com/deepin/anything",
                                                       QDBusConnection::systemBus(),
                                                       this);
}

AnythingSearcher::~AnythingSearcher()
{
}

bool AnythingSearcher::search()
{
    //准备状态切运行中，否则直接返回
    if (!status.testAndSetRelease(kReady, kRuning))
        return false;

    QStringList searchDirList;
    auto path = UrlRoute::urlToPath(searchUrl);
    if (isPrependData)
        path.prepend("/data");

    if (path.isEmpty() || keyword.isEmpty()) {
        status.storeRelease(kCompleted);
        return false;
    }

    notifyTimer.start();
    // 如果挂载在此路径下的其它目录也支持索引数据, 则一并搜索
    QStringList dirs = anythingInterface->hasLFTSubdirectories(path);
    searchDirList << dirs;
    if (searchDirList.isEmpty() || searchDirList.first() != path)
        searchDirList.prepend(path);

    uint32_t startOffset = 0;
    uint32_t endOffset = 0;
    while (!searchDirList.isEmpty()) {
        //中断
        if (status.loadAcquire() != kRuning)
            return false;

        const auto &reply = anythingInterface->search(kMaxCount, kMaxTime, startOffset, endOffset, searchDirList.first(), keyword, true);
        auto results = reply.argumentAt<0>();
        if (reply.error().type() != QDBusError::NoError) {
            qWarning() << "deepin-anything search failed:"
                       << QDBusError::errorString(reply.error().type())
                       << reply.error().message();
            startOffset = endOffset = 0;
            searchDirList.removeAt(0);
            continue;
        }

        startOffset = reply.argumentAt<1>();
        endOffset = reply.argumentAt<2>();

        // 当前目录已经搜索到了结尾
        if (startOffset >= endOffset) {
            startOffset = endOffset = 0;
            searchDirList.removeAt(0);
        }

        for (auto &item : results) {
            // 中断
            if (status.loadAcquire() != kRuning)
                return false;

            // 去除掉添加的data前缀
            if (isPrependData && item.startsWith("/data"))
                item = item.mid(5);

            {
                QMutexLocker lk(&mutex);
                allResults << QUrl::fromLocalFile(item);
            }

            // 推送
            tryNotify();
        }
    }

    //检查是否还有数据
    if (status.testAndSetRelease(kRuning, kCompleted)) {
        //发送数据
        if (hasItem())
            emit unearthed(this);
    }

    return true;
}

void AnythingSearcher::stop()
{
    status.storeRelease(kTerminated);
}

bool AnythingSearcher::hasItem() const
{
    QMutexLocker lk(&mutex);
    return !allResults.isEmpty();
}

QList<QUrl> AnythingSearcher::takeAll()
{
    QMutexLocker lk(&mutex);
    return std::move(allResults);
}

void AnythingSearcher::tryNotify()
{
    int cur = notifyTimer.elapsed();
    if (hasItem() && (cur - lastEmit) > kEmitInterval) {
        lastEmit = cur;
        qDebug() << "unearthed, current spend:" << cur;
        emit unearthed(this);
    }
}

bool AnythingSearcher::isSupported(const QUrl &url, bool &isPrependData)
{
    if (!url.isValid() || UrlRoute::isVirtual(url))
        return false;

    static ComDeepinAnythingInterface anything("com.deepin.anything",
                                               "/com/deepin/anything",
                                               QDBusConnection::systemBus());
    if (!anything.isValid())
        return false;

    auto path = UrlRoute::urlToPath(url);
    if (!anything.hasLFT(path)) {
        if (path.startsWith("/home") && QDir().exists("/data/home")) {
            path.prepend("/data");
            if (!anything.hasLFT(path))
                return false;
            isPrependData = true;
        } else {
            return false;
        }
    }

    return true;
}

DSB_FM_END_NAMESPACE
