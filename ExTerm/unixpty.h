#ifndef UNIXPTY_H
#define UNIXPTY_H

#include "basepty.h"
#include <pty.h>

class UnixPty : public BasePty
{
    Q_OBJECT
    struct winsize winSize;

    bool m_cursorKeyMode;
protected:
    int masterFd;
    char readBuffer[4096];
public:
    explicit UnixPty(QSize size, QObject *parent = 0);

    Q_PROPERTY(bool cursorKeyMode READ cursorKeyMode WRITE setCursorKeyMode)

    bool cursorKeyMode() const;

    void setSize(const QSize &size);
    void setPixelSize(const QSize &size);
    void sendKey(const QString &text, Qt::Key key, Qt::KeyboardModifiers modifiers);

    void write(const QByteArray &data);
public slots:
    void setCursorKeyMode(bool state);
};

#endif // UNIXPTY_H
