#ifndef SCREENMODEL_H
#define SCREENMODEL_H

#include <QObject>
#include <QSize>
#include <QPoint>

#include <QQueue>
#include <QVector>
#include <QChar>
#include <QColor>
#include <QLinkedList>

#include <functional>

#include <QDebug>

class ScreenStyle
{
public:
    enum Flags {
        DefaultFg = 0x1,
        DefaultBg = 0x2,
        Bold = 0x4,
        Underline = 0x8,
        Inverse = 0x10,
        Blinking = 0x20
    };
private:
    QColor m_fgColor, m_bgColor;

    quint8 flags : 8;

public:
    ScreenStyle(QColor fgColor, QColor bgColor, quint8 _flags = 0)
        : m_fgColor(fgColor), m_bgColor(bgColor),
          flags(_flags) {}
    ScreenStyle() : flags(DefaultFg|DefaultBg) {}

    void reset() {
        *this = ScreenStyle();
    }

    void setFgColor(QColor fgColor) {
        m_fgColor = fgColor;
        flags &= ~DefaultFg;
    }
    QColor fgColor() const {
      return m_fgColor;
    };
    void setBgColor(QColor bgColor) {
        m_bgColor = bgColor;
        flags &= ~DefaultBg;
    }
    QColor bgColor() const {
      return m_bgColor;
    };
    bool isDefault() const {
        return (flags == (Flags::DefaultBg|Flags::DefaultFg));
    };

    bool hasFlag(Flags flag) const {
        return flags & (quint8)flag;
    }
    void setFlag(Flags flag) {
        flags |= flag;
    }
    void clearFlag(Flags flag) {
        flags &= ~flag;
    }

    bool isCompatible(const ScreenStyle& other) const {
        if(fgColor() == other.fgColor() && bgColor() == other.bgColor() && flags == other.flags)
            return true;
        return false;
    };
};

struct StyleEntry : public ScreenStyle
{
    quint16 from, to;
    StyleEntry(const ScreenStyle& style) : ScreenStyle(style) { }
};

struct ScreenChar
{
    QChar ch;
    ScreenStyle style;

    ScreenChar(QChar _ch, const ScreenStyle& _style) : style(_style) {}
    ScreenChar(QChar _ch) : ch(_ch) {}
};

class ScreenLine
{
public:
    enum class LineMode : char {
        DoubleHTop = 1,
        DoubleHBottom = 2,
        DoubleHWTop = 3,
        DoubleHWBottom = 4,
        NormalW = 5,
        DoubleW = 6
    };
private:
    void assureLength(int length) {
        if(length > m_data.length()) {
            QString str(length - m_data.length(), ' ');
            m_data.append(str);
        }
    }
    quint64 m_id;
    LineMode m_lineMode = LineMode::NormalW;
    bool m_wrapped : 8;
    QString m_data;

    typedef QLinkedList<StyleEntry> StyleEntryList;
    StyleEntryList styles;

    StyleEntryList::iterator iteratorFor(int from, int to, const ScreenStyle& style = ScreenStyle());

    void optimizeStyles();
public:
    ScreenLine(uint id, bool wrapped = false) : m_id(id), m_wrapped(wrapped) {
        ;
    }

    bool wrapped() const {
        return m_wrapped;
    }

    bool clear() {
        if(!m_data.length())
            return false;
        m_data.clear();
        styles.clear();
        return true;
    }

    const QString& data() const {
        return m_data;
    }

    quint64 id() const {
        return m_id;
    }

    void setLineMode(LineMode mode) {
        m_lineMode = mode;
    }

    LineMode lineMode() const {
        return m_lineMode;
    }

    void render(std::function<void(int from, int to, const ScreenStyle& style)> f) const {
        int lastTo = 0;
        for(auto i = styles.begin();i != styles.end();++i) {
            if(lastTo < i->from)
                f(lastTo, i->from-1, ScreenStyle());
            f(i->from, i->to, *i);
            lastTo = i->to + 1;
        }
        if(lastTo < m_data.length() -1) {
            f(lastTo, m_data.length() -1, ScreenStyle());
        }
    }

    bool isDoubleWidth() const {
        return m_lineMode == LineMode::DoubleHWTop ||
                m_lineMode == LineMode::DoubleHWBottom ||
                m_lineMode == LineMode::DoubleW;
    }
    bool isDoubleHeightTop() const {
        return m_lineMode == LineMode::DoubleHTop ||
                m_lineMode == LineMode::DoubleHWTop;
    }
    bool isDoubleHeightBottom() const {
        return m_lineMode == LineMode::DoubleHBottom ||
                m_lineMode == LineMode::DoubleHWBottom;
    }

    ScreenChar operator[](int idx) const {
        if(idx >= m_data.length())
            return ScreenChar(' ');
        if(styles.size() == 0)
            return ScreenChar(m_data[idx]);
        for(auto style : styles) {
            if(style.from >= idx && style.to <= idx)
                return ScreenChar(m_data[idx], style);
        }
        return ScreenChar(' ');
    };

    void setChar(int idx, const ScreenChar& screenChar) {
        assureLength(idx + 1);
        m_data[idx] = screenChar.ch;
        // TODO: set style
    };

    void insertAtPos(int pos, const QString& text, const ScreenStyle& style = ScreenStyle()) {
        assureLength(pos + text.length() + 1);
        m_data.replace(pos, text.size(), text);

        if(styles.size() || !style.isDefault()) {
            auto i = iteratorFor(pos, pos+text.length()-1, style);
            if(i->isDefault())
                styles.erase(i);
            optimizeStyles();
        }
    }
    bool eraseFromPos(int pos) {
        if(pos >= m_data.length())
            return false;
        pos = qMin(m_data.length(), pos);

        styles.erase(iteratorFor(pos, -1));
        m_data = m_data.left(pos);
        return true;
    }
    void deleteRight(int pos, int n) {
        styles.erase(iteratorFor(pos, pos + n - 1));
        for(auto style: styles) {
            if(style.from > pos)
                style.from -= n;
            if(style.to > pos)
                style.to -= n;
        }
        m_data.remove(pos, n);
    }
};


class ScreenModel : public QObject
{
    Q_OBJECT
    QSize m_size;
    QPoint m_cursor;

    quint64 lineIdCounter = 1000;
    quint64 discardedLines = 1000;
    QQueue<ScreenLine> lines;
    bool m_cursorVisible = true;
    bool m_cursorBlinking = false;
    ScreenStyle m_style;
public:
    explicit ScreenModel(QObject *parent = 0);
    explicit ScreenModel(const QSize& initialSize, QObject *parent = 0);

    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QPoint cursor READ cursor WRITE setCursor NOTIFY cursorChanged)

    Q_PROPERTY(bool cursorVisible READ cursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(bool cursorBlinking READ cursorBlinking WRITE setCursorBlinking NOTIFY cursorBlinkingChanged)

    QSize size();
    void setSize(QSize size);
    QPoint cursor();
    void setCursor(QPoint cursor);

    ScreenLine& operator[](int idx) { return lines[idx]; }
    const ScreenLine& operator[](int idx) const { return lines[idx]; }

    ScreenLine& currentLine();
    ScreenStyle& style() {
        return m_style;
    }
    const ScreenStyle& style() const {
        return m_style;
    }

    int getLineIdxById(quint64 id);

    void setLineMode(ScreenLine::LineMode lineMode);

    void eraseFromCursor();

    bool cursorBlinking() const;
    void setCursorBlinking(bool cursorBlinking);
    bool cursorVisible() const;
    void setCursorVisible(bool cursorVisible);


    void insertAtCursor(const QString& text);
    void moveCursorHome();
    void moveCursorRight(int n = 1);
    void moveCursorLeft(int n = 1);
    void moveCursorUp(int n = 1);
    void moveCursorDown(int n = 1);

    void deleteRight(int n = 1);

    void moveCursorTo(const QPoint& coordinates);

    void clearToBeginningOfScreen();
    void clearToEndOfScreen();
    void clearScreen();

    void lineFeed(bool lineWrapped = false);
    void backspace();
signals:
    void cursorChanged(const QPoint& cursor);
    void sizeChanged(const QSize& size);
    void cursorVisibleChanged(bool cursorVisible);
    void cursorBlinkingChanged(bool cursorBlinking);

    void screenChanged();
    void lineChanged(int idx);

    void onLineFeed();
public slots:

};

#endif // SCREENMODEL_H
