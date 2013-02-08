#include "extermio.h"

#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QtConcurrentRun>
#include <QQuickItem>
#include <QQmlEngine>
#include <QThread>
#include <QMimeDatabase>

static QMimeDatabase mimeDatabase;

ExTermIO::ExTermIO(QObject *parent) :
    QObject(parent)
{
}

void ExTermIO::getUrl(const QString &urlString, QJSValue callback) {
    QUrl url(urlString, QUrl::StrictMode);
    QJSValueList args;
    QJSValue error;
    args << error;
    if(!url.isValid()) {
        error = QJSValue(url.errorString());
        callback.call(args);
        return;
    }


    auto readData = [=]() {

        qDebug() << "Running in thread" << QThread::currentThreadId();
        QFile f(url.path());
        f.open(QFile::ReadOnly);

        if(!f.isOpen()) {
            qDebug() << "File not open";
            return QByteArray();
        }
        qDebug() << "File is open";
        auto res = f.readAll();
        qDebug() << "File read finished";
        return res;
    };

#ifdef USE_ASYNC
    qDebug() << "Main thread is thread" << QThread::currentThreadId();;
    auto res = QtConcurrent::run(readData);

    auto watcher = new QFutureWatcher<QByteArray>;
    qDebug() << "Binding watcher";
    connect(watcher, &QFutureWatcher<QByteArray>::finished, [=,&callback]() {
        qDebug() << "Got result in thread" << QThread::currentThreadId();;
        QQmlEngine* engine = qmlEngine(this);
        QJSValueList args;
        args << QJSValue();
        args << engine->toScriptValue(res.result());
        watcher->deleteLater();

        qDebug() << "Is callable" << callback.isCallable();
        qDebug() << "ddd";

        callback.call(args);
    });
    watcher->setFuture(res);
#else
    QQmlEngine* engine = qmlEngine(this);
    QByteArray data = readData();
    args << engine->toScriptValue(data);
    args << mimeDatabase.mimeTypeForFileNameAndData(url.path(), data).name();
    callback.call(args);
#endif
}
