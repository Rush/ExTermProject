#ifndef BASEPTY_H
#define BASEPTY_H

#include <QObject>
#include <QSize>

class BasePty : public QObject
{
    Q_OBJECT
    QSize m_size;
    QSize m_pixelSize;
public:
    explicit BasePty(QSize& size, QObject *parent = 0);
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QSize size READ pixelSize WRITE setPixelSize NOTIFY sizeChanged)

    virtual QSize size();
    virtual void setSize(const QSize& size);
    virtual QSize pixelSize();
    virtual void setPixelSize(const QSize& size);

    virtual void write(const QByteArray &data) = 0;
    virtual void sendKey(const QString &text, Qt::Key key, Qt::KeyboardModifiers modifiers) = 0;
signals:
    void hangupReceived();
    void readyRead(const QByteArray &data);
    void sizeChanged(QSize size, QSize pixelSize);
};

#endif // BASEPTY_H
