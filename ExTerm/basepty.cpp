#include "basepty.h"

BasePty::BasePty(QSize& size, QObject *parent) :
    QObject(parent), m_size(size)
{
}

void BasePty::setSize(const QSize &size)
{
    if(size == m_size)
        return;
    m_size = size;
    emit sizeChanged(m_size, m_pixelSize);
}

QSize BasePty::size()
{
    return m_size;
}


void BasePty::setPixelSize(const QSize &pixelSize)
{
    if(pixelSize == m_pixelSize)
        return;
    m_pixelSize = pixelSize;
    emit sizeChanged(m_size, m_pixelSize);
}

QSize BasePty::pixelSize()
{
    return m_pixelSize;
}
