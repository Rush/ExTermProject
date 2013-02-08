#ifndef EXTERMITEM_H
#define EXTERMITEM_H

#include <QQuickItem>
#include <QFontMetricsF>
#include "screenmodel.h"
#include "basepty.h"

#include <QMap>
#include <QSet>
#include <QTimer>

class QQmlComponent;
#include <QDebug>

class ExTermItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(qreal charWidth READ charWidth NOTIFY charWidthChanged)
    Q_PROPERTY(qreal lineHeight READ lineHeight NOTIFY lineHeightChanged)
//    Q_PROPERTY(int height READ height WRITE setHeight)
//    Q_PROPERTY(int width READ width WRITE setWidth)
//    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY screenTitleChanged)
//    Q_PROPERTY(bool cursorVisible READ cursorVisible NOTIFY cursorVisibleChanged)
//    Q_PROPERTY(bool cursorBlinking READ cursorBlinking NOTIFY cursorBlinkingChanged)
//    Q_PROPERTY(bool selectionEnabled READ selectionEnabled NOTIFY selectionEnabledChanged)
//    Q_PROPERTY(QPointF selectionAreaStart READ selectionAreaStart WRITE setSelectionAreaStart NOTIFY selectionAreaStartChanged)
//    Q_PROPERTY(QPointF selectionAreaEnd READ selectionAreaEnd WRITE setSelectionAreaEnd NOTIFY selectionAreaEndChanged)


    QTimer renderTimer;

    BasePty* pty;
    QFont m_font;
    QQmlEngine* engine;
    ScreenModel model;
    QFontMetricsF fontMetrics;

    QMap<quint64, QQuickItem*> renderedLines;
    bool scheduleFullRedraw = false;
    QSet<quint64> changedLines;

    QMap<quint64, QQuickItem*> htmlItems;
    QMap<QString, quint64> objectId2Line;

    void initializeView();
    void deinitializeView();

    QQmlComponent* htmlItemComponent = NULL;
    QQmlComponent* textSegmentComponent = NULL;
    QQmlComponent* lineItemComponent = NULL;

    QQuickItem* screenItem = NULL;
    QQuickItem* cursorItem = NULL;

    QPointF mapCursorToPosition(const QPoint& cursor);

    QSet<QQuickItem*> spareLineItems;
    QQuickItem* createLineItem();
    void deleteLineItem(QQuickItem* lineItem, quint64 id);

    QSet<QQuickItem*> spareTextSegments;
    QQuickItem* createTextSegment(QQuickItem* lineItem);
    void deleteTextSegment(QQuickItem* textSegment);

public:
    explicit ExTermItem(QQuickItem *parent = 0);

    const QFont& font() const;

    qreal lineHeight() const;
    qreal charWidth() const;
    void setFont(const QFont& font);

    QObject* renderLine(qreal offset, const ScreenLine& line);
    void renderAll();

    void itemChange(ItemChange, const ItemChangeData &);

    Q_INVOKABLE void onKeyPressed(const QString &text, Qt::Key key, Qt::KeyboardModifiers modifiers);

private slots:
    void clearRenderedCache();


    void onLineChanged(int idx);
    void onCursorChanged(const QPoint& cursor);
    void onLineFeed();

    void createHtmlItem(const QString& objectId, int rows, const QString& data);

    void onDiscardedLine(quint64 id);

signals:
    void fontChanged(QFont);
    void lineHeightChanged(qreal);
    void charWidthChanged(qreal);


public slots:
};

#endif // EXTERMITEM_H
