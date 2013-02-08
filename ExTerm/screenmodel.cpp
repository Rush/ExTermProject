#include "screenmodel.h"

#include <QDebug>

static const int LINE_LIMIT = 128;


ScreenModel::ScreenModel(const QSize& initialSize, QObject *parent) :
    QObject(parent), lines()
{
    setSize(initialSize);
}

ScreenModel::ScreenModel(QObject *parent) : ScreenModel(QSize(80, 25), parent) {

}

void ScreenLine::optimizeStyles()
{
    int cnt = 0;
    for(auto i = styles.begin();i != styles.end();) {
        auto prev = i;
        auto next = ++i;
        if(next == styles.end())
            break;
        if(prev->isCompatible(*next)) {
            next->from = prev->from;
            styles.erase(prev);
            cnt++;
        }
    }
}

ScreenLine::StyleEntryList::iterator ScreenLine::iteratorFor(int from, int to, const ScreenStyle& style) {
    auto i = styles.begin();
    auto entry = StyleEntry(style);
    if(styles.size()) {
        if(from == -1)
            from = styles.first().from;
        if(to == -1)
            to = styles.last().to;
    }

    entry.from = from;
    entry.to = to;

    auto end = styles.end();
    StyleEntryList::iterator fromI = end, toI = end;

    // find element from which we will need to check and/or remove
    for(;i != styles.end();++i) {
        if(i->from < from) {
            fromI = StyleEntryList::iterator(i);
        }
    }
    for(i=styles.begin();i != styles.end();++i) {
        if(i->to > to) {
            toI = StyleEntryList::iterator(i);
            break;
        }
    }

    // override all elements or insert into empty list
    if(fromI == end && toI == end) {
        styles.clear();
        return styles.insert(styles.end(), entry);
    }
    // insert inside element
    else if(fromI == toI) {
        StyleEntry copy = *toI;
        fromI->to = from-1;
        copy.from = to+1;
        auto second = styles.insert(fromI+1, copy);
        auto res = styles.insert(second, entry);

        return res;
    }
    else {
        if(fromI != styles.end())
            fromI->to = qMin((int)fromI->to, from-1);
        if(toI != styles.end())
            toI->from = qMax(to+1, (int)toI->from);
        styles.erase(fromI+1, toI);
        return styles.insert(toI, entry);
    }

    return styles.end();
};

QPoint ScreenModel::cursor()
{
    return m_cursor;
}

void ScreenModel::setCursor(QPoint cursor)
{
    if(cursor == m_cursor)
        return;
    m_cursor = cursor;
    emit cursorChanged(cursor);
}

QSize ScreenModel::size()
{
    return m_size;
}

void ScreenModel::setSize(QSize size)
{
    if(size == m_size)
        return;

    quint64 lastId = lines.isEmpty() ? lineIdCounter: lines.last().id();
    while(size.height() > lines.size()) {
        lines.append(ScreenLine(--lastId));
        discardedLines--;
    }
    m_size = size;
    emit sizeChanged(size);
}

ScreenLine& ScreenModel::currentLine()
{
    return lines[m_cursor.y()];
}

void ScreenModel::eraseFromCursor()
{
    if(style().isDefault()) {
        if(currentLine().eraseFromPos(m_cursor.x()))
            emit lineChanged(m_cursor.y());
    }
    else {
        int n = m_size.width() - m_cursor.x();
        currentLine().insertAtPos(m_cursor.x(), QString(n, ' '), style());
        emit lineChanged(m_cursor.y());
    }
}

void ScreenModel::insertAtCursor(const QString &text)
{
    if(text.length() == 0)
        return;
    //qDebug() << "Insert at " << cursor() << text;

    int lengthTillEnd = m_size.width() - m_cursor.x() ;
    auto part1 = text.leftRef(lengthTillEnd);

    currentLine().insertAtPos(m_cursor.x(), part1.toString(), style());
    m_cursor.rx() += text.length();

    emit lineChanged(m_cursor.y());
    emit cursorChanged(m_cursor);

    if(text.length() > part1.length()) {
        auto part2 = text.rightRef(text.length() - part1.length());
        if(m_cursor.y() == 0)
            lineFeed(true);
        insertAtCursor(part2.toString());
    }
}

int ScreenModel::getLineIdxById(quint64 id)
{
    int idx = discardedLines + lines.length()-1 - id;
    if(idx >= 0 && idx < lines.length()) {
        return idx;
    }
    return -1;
}

void ScreenModel::backspace()
{
    moveCursorLeft();
}

void ScreenModel::lineFeed(bool lineWrapped)
{
    //qDebug() << "Line feed" << cursor();
    if(m_cursor.y() == 0) {
        if(lines.size() == LINE_LIMIT) {

            emit discardedLine(lines.last().id());
            lines.removeLast();
            discardedLines++;
        }

        lines.prepend(ScreenLine(lineIdCounter++, lineWrapped));
        moveCursorHome();

        emit onLineFeed();    // TODO change naming
    }
    else {
        m_cursor.ry() -= 1;
        m_cursor.rx() = 0;
        emit cursorChanged(m_cursor);
    }
}

void ScreenModel::moveCursorTo(const QPoint& coordinates)
{
    //qDebug() << "Move cursor to" << coordinates;
    QPoint oldCursor = m_cursor;
    m_cursor.rx() = coordinates.x() - 1;
    m_cursor.ry() = m_size.height() - coordinates.y();
    m_cursor.rx() = qMin(qMax(0, m_cursor.x()), m_size.width() - 1);
    m_cursor.ry() = qMin(qMax(0, m_cursor.y()), m_size.height() - 1);
    if(m_cursor != oldCursor)
        emit cursorChanged(m_cursor);
}

void ScreenModel::moveCursorRight(int n)
{
    //qDebug() << "Move cursor right"<< n;
    if(m_cursor.x() < m_size.width()-1) {
        m_cursor.rx() += n;
        m_cursor.rx() = qMin(m_size.width()-1, m_cursor.x());
        emit cursorChanged(m_cursor);
    }
}

void ScreenModel::moveCursorLeft(int n)
{
    //qDebug() << "Move cursor left"<< n;
    n = qMin(m_cursor.x(), n);
    if(n) {
        m_cursor.rx() -= n;
        emit cursorChanged(m_cursor);
    }
}

void ScreenModel::moveCursorHome()
{
    moveCursorLeft(m_cursor.x());
}

void ScreenModel::moveCursorUp(int n)
{
    //qDebug() << "Move cursor up" << n;
    int old = m_cursor.y();
    m_cursor.ry() = qMin(m_size.height()-1, m_cursor.y() + n);
    if(m_cursor.y() != old)
        emit cursorChanged(m_cursor);
}

void ScreenModel::moveCursorDown(int n)
{
    //qDebug() << "Move cursor down"<< n;
    n = qMin(m_cursor.y(), n);
    if(n) {
        m_cursor.ry() -= n;
        emit cursorChanged(m_cursor);
    }
}

void ScreenModel::deleteRight(int n)
{
    currentLine().deleteRight(m_cursor.x(), n);
}

void ScreenModel::setLineMode(ScreenLine::LineMode lineMode)
{
    if(currentLine().lineMode() == lineMode)
        return;
    currentLine().setLineMode(lineMode);
    emit lineChanged(m_cursor.y());
}

void ScreenModel::clearScreen()
{
    //qDebug() << "Clear screen";
    for(int i = 0; i < m_size.height();++i) {
        lines[i].clear();
        emit lineChanged(i);
    }
}

void ScreenModel::clearToEndOfScreen()
{
    //qDebug() << "Clear to end of screen";
    for(int i = 0; i <= m_cursor.y();++i) {
        lines[i].clear();
        emit lineChanged(i);
    }
}

void ScreenModel::clearToBeginningOfScreen()
{
    //qDebug() << "Clear to beginning of screen";
    for(int i = m_cursor.y(); i < m_size.height();++i) {
        lines[i].clear();
        emit lineChanged(i);
    }
}

bool ScreenModel::cursorBlinking() const
{
    return m_cursorBlinking;
}

bool ScreenModel::cursorVisible() const
{
    return m_cursorVisible;
}

void ScreenModel::setCursorBlinking(bool cursorBlinking)
{
    if(m_cursorBlinking == cursorBlinking)
        return;
    m_cursorBlinking = cursorBlinking;
    emit cursorBlinkingChanged(cursorBlinking);
}

void ScreenModel::setCursorVisible(bool cursorVisible)
{
    if(m_cursorVisible == cursorVisible)
        return;
    m_cursorVisible = cursorVisible;
    emit cursorVisibleChanged(cursorVisible);
}
