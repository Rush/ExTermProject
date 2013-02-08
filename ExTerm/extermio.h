#ifndef EXTERMIO_H
#define EXTERMIO_H

#include <QObject>
#include <QJSValue>
#include <QFutureWatcher>

class ExTermIO : public QObject
{
    Q_OBJECT

public:
    explicit ExTermIO(QObject *parent = 0);

    Q_INVOKABLE void getUrl(const QString& url, QJSValue callback);
signals:

public slots:

};

#endif // EXTERMIO_H
