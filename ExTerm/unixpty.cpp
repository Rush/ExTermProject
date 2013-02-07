#include "unixpty.h"
#include <pty.h>
#include <QString>
#include <unistd.h>
#include <QSocketNotifier>

#include <QDebug>

#include "controlchars.h"

UnixPty::UnixPty(QSize size, QObject *parent) :
    BasePty(size, parent), winSize((struct winsize){0,})
{
    int terminalPid = forkpty(&masterFd,
                             NULL,
                             NULL,
                             NULL);
    if (terminalPid == 0) {
        setenv("COLUMNS", qPrintable(QString::number(size.width())), 0);
        setenv("LINES", qPrintable(QString::number(size.height())), 0);

        ::execl("/bin/bash", "/bin/bash", (const char *) 0);
    }

    auto reader = new QSocketNotifier(masterFd,QSocketNotifier::Read, this);
    connect(reader, &QSocketNotifier::activated, [=](int socket) {
        int readSize = ::read(masterFd, readBuffer, sizeof(readBuffer));
        emit readyRead(QByteArray::fromRawData(readBuffer, readSize));
    });

    setSize(size);
}

void UnixPty::setSize(const QSize &size)
{
    winSize.ws_col = size.width();
    winSize.ws_row = size.height();
    ioctl(masterFd, TIOCSWINSZ, &winSize);
    BasePty::setSize(size);
}

void UnixPty::setPixelSize(const QSize &size)
{
    winSize.ws_xpixel = size.width();
    winSize.ws_ypixel = size.height();
    ioctl(masterFd, TIOCSWINSZ, &winSize);
    BasePty::setPixelSize(size);
}

void UnixPty::write(const QByteArray &data)
{
    if (::write(masterFd, data.constData(), data.size()) < 0) {
        qDebug() << "Something whent wrong when writing to masterFd";
    }
}

bool UnixPty::cursorKeyMode() const
{
    return m_cursorKeyMode;
}

void UnixPty::setCursorKeyMode(bool state)
{
    m_cursorKeyMode = state;
}

void UnixPty::sendKey(const QString &text, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    char escape = '\0';
    char  control = '\0';
    char  code = '\0';
    QVector<ushort> parameters;
    bool found = true;

    switch(key) {
    case Qt::Key_Up:
        escape = C0::ESC;
        if (m_cursorKeyMode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

        code = 'A';
        break;
    case Qt::Key_Right:
        escape = C0::ESC;
        if (m_cursorKeyMode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

        code = 'C';
        break;
    case Qt::Key_Down:
        escape = C0::ESC;
        if (m_cursorKeyMode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

            code = 'B';
        break;
    case Qt::Key_Left:
        escape = C0::ESC;
        if (m_cursorKeyMode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

        code = 'D';
        break;
    case Qt::Key_Insert:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(2);
        code = '~';
        break;
    case Qt::Key_Delete:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(3);
        code = '~';
        break;
    case Qt::Key_Home:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(1);
        code = '~';
        break;
    case Qt::Key_End:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(4);
        code = '~';
        break;
    case Qt::Key_PageUp:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(5);
        code = '~';
        break;
    case Qt::Key_PageDown:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(6);
        code = '~';
        break;
    case Qt::Key_F1:
    case Qt::Key_F2:
    case Qt::Key_F3:
    case Qt::Key_F4:
        if (m_cursorKeyMode) {
            parameters.append((key & 0xff) - 37);
            escape = C0::ESC;
            control = C1_7bit::CSI;
            code = '~';
        }
        break;
    case Qt::Key_F5:
    case Qt::Key_F6:
    case Qt::Key_F7:
    case Qt::Key_F8:
    case Qt::Key_F9:
    case Qt::Key_F10:
    case Qt::Key_F11:
    case Qt::Key_F12:
        if (m_cursorKeyMode) {
            parameters.append((key & 0xff) - 36);
            escape = C0::ESC;
            control = C1_7bit::CSI;
            code = '~';
        }
        break;
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
        return;
        break;
    default:
        found = false;
    }

    if (found) {
        int term_mods = 0;
        if (modifiers & Qt::ShiftModifier)
            term_mods |= 1;
        if (modifiers & Qt::AltModifier)
            term_mods |= 2;
        if (modifiers & Qt::ControlModifier)
            term_mods |= 4;

        QByteArray toPty;

        if (term_mods) {
            term_mods++;
            parameters.append(term_mods);
        }
        if (escape)
            toPty.append(escape);
        if (control)
            toPty.append(control);
        if (parameters.size()) {
            for (int i = 0; i < parameters.size(); i++) {
                if (i)
                    toPty.append(';');
                toPty.append(QByteArray::number(parameters.at(i)));
            }
        }
        if (code)
            toPty.append(code);
        write(toPty);

    } else {
        QByteArray to_pty;
        QByteArray key_text;
        if (modifiers & Qt::ControlModifier) {
            char key_char = text.toLocal8Bit().at(0);
            key_text.append(key_char & 0x1F);

        } else {
            key_text = text.toUtf8();
        }

        if (modifiers &  Qt::AltModifier) {
            to_pty.append(C0::ESC);
        } else if (modifiers & Qt::MetaModifier) {
            to_pty.append(C0::ESC);
            to_pty.append('@');
            to_pty.append(FinalBytesNoIntermediate::Reserved3);
        }

        to_pty.append(key_text);
        write(to_pty);
    }
}
