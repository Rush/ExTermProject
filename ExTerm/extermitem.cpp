#include "extermitem.h"

#include <QQuickWindow>
#include <QQmlComponent>

#include <QDebug>

#include "unixpty.h"

#include "screenmodel.h"
#include "vt100parser.h"

#include <QElapsedTimer>

ExTermItem::ExTermItem(QQuickItem *parent) :
    QQuickItem(parent), fontMetrics(m_font), lineItemComponent(NULL),
    textSegmentComponent(NULL)
{
    pty = new UnixPty(QSize(80, 25), this);

    VT100Parser* parser = new VT100Parser(&model);
    connect(pty, &UnixPty::readyRead, parser, &VT100Parser::onData);

    connect(parser, &VT100Parser::cursorKeyModeChanged,
            (UnixPty*)pty, &UnixPty::setCursorKeyMode);

    connect(&model, &ScreenModel::sizeChanged, [&](const QSize& size) {
        pty->setSize(size);
        pty->setPixelSize(QSize(size.width() * charWidth(),
                                size.height() * lineHeight()));
    });

    connect(parser, &VT100Parser::htmlBlock,
            this, &ExTermItem::createHtmlItem);
}

void ExTermItem::createHtmlItem(const QString &objectId, int rows, const QString &data)
{
    if(!htmlItemComponent || !htmlItemComponent->isReady()) {
        model.insertAtCursor("ExTerm: HTML component not ready");
        return;
    }
    qDebug() << m_font.pointSizeF();
    qDebug() << m_font.pixelSize();

    QQuickItem* htmlItem = qobject_cast<QQuickItem*>(htmlItemComponent->beginCreate(qmlContext(this)));
    quint64 currentLineId = model.currentLine().id();
    htmlItem->setParentItem(this);
    htmlItem->setHeight(rows * lineHeight());
    auto pos = mapCursorToPosition(QPoint(0, 0));
    pos.ry() -= htmlItem->height() + lineHeight();
    htmlItem->setY(pos.y());

    htmlItem->setObjectName(objectId);

    htmlItemComponent->completeCreate();
    htmlItems[currentLineId] = htmlItem;
    objectId2Line[objectId] = currentLineId;

    QFile templateFile(":html/template.html");
    templateFile.open(QFile::ReadOnly);
    QString htmlTemplate = templateFile.readAll();

    htmlTemplate = htmlTemplate.replace("{fontColor}","white");
    htmlTemplate = htmlTemplate.replace("{content}", data);
    htmlTemplate = htmlTemplate.replace("{fontFamily}", QChar('"') + m_font.family() + QChar('"'));

    htmlTemplate = htmlTemplate.replace("{fontSize}", QString("%1").arg(m_font.pixelSize()));

    //qDebug() << htmlTemplate;
    htmlItem->setProperty("html", htmlTemplate);
}

QQuickItem* ExTermItem::createLineItem()
{
    if(spareLineItems.size()) {
        QQuickItem* result = *(spareLineItems.begin());
        spareLineItems.erase(spareLineItems.begin());
        result->setVisible(true);
        return result;
    }

    QQuickItem* lineItem = qobject_cast<QQuickItem*>(lineItemComponent->beginCreate(qmlContext(this)));
    lineItem->setParentItem(this);
    return lineItem;
}

void ExTermItem::deleteLineItem(QQuickItem* lineItem, quint64 id)
{
    lineItem->setVisible(false);

    int idx = model.getLineIdxById(id);
    if(idx == -1) {
        if(htmlItems.contains(id)) {
            QQuickItem* htmlItem = htmlItems[id];
            htmlItem->setParentItem(NULL);
            objectId2Line.remove(htmlItem->objectName());
            htmlItem->deleteLater();
        }
    }


    for(auto child: lineItem->childItems()) {
//        //child->deleteLater();
        deleteTextSegment(child);

    }
    spareLineItems.insert(lineItem);
    //    lineItem->deleteLater();
}

QQuickItem* ExTermItem::createTextSegment(QQuickItem* lineItem)
{
    if(spareTextSegments.size()) {
        QQuickItem* result = *(spareTextSegments.begin());
        spareTextSegments.erase(spareTextSegments.begin());
        result->setParentItem(lineItem);
        return result;
    }

    QQuickItem* textSegment = qobject_cast<QQuickItem*>(textSegmentComponent->beginCreate(qmlContext(this)));
    textSegment->setParentItem(lineItem);
    textSegmentComponent->completeCreate();
    return textSegment;
}

void ExTermItem::deleteTextSegment(QQuickItem *textSegment)
{
//    textSegment->deleteLater();
    //textSegment->setVisible(false);
    textSegment->setParentItem(NULL);
    spareTextSegments.insert(textSegment);
}


void ExTermItem::renderAll()
{
    if(!lineItemComponent)
        return;
    if(!lineItemComponent->isReady())
        return;

    int lines = height() / lineHeight();// + 1;
    for(auto i = renderedLines.begin();i != renderedLines.end();) {
        int idx = model.getLineIdxById(i.key());

        if(idx >= lines || idx == -1) {
            deleteLineItem(i.value(), i.key());
            i = renderedLines.erase(i);
            continue;
        }
        i++;
    }

    for(int i = 0;i < lines;++i) {
        auto pos = mapCursorToPosition(QPoint(0, i));
        renderLine(pos.y(), model[i]);
    }
}

void ExTermItem::onKeyPressed(const QString &text, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    pty->sendKey(text, key, modifiers);
    if(renderTimer.interval() > 5 && !renderTimer.isActive())
        renderTimer.start(5);
}

const QFont& ExTermItem::font() const
{
    return m_font;
}

qreal ExTermItem::lineHeight() const
{
    return fontMetrics.lineSpacing();
}

qreal ExTermItem::charWidth() const
{
    return fontMetrics.averageCharWidth();
}

void ExTermItem::setFont(const QFont& font)
{
    m_font = font;
    m_font.setHintingPreference(QFont::PreferNoHinting);
    m_font.setBold(false);
    fontMetrics = QFontMetricsF(font);
    //qDebug() << "Set font" <<
    //fontMetrics.av
    clearRenderedCache();
    emit lineHeightChanged(lineHeight());
    emit charWidthChanged(charWidth());
    emit fontChanged(font);
}

QObject* ExTermItem::renderLine(qreal offset, const ScreenLine &line)
{
    uint id = line.id();

    if(htmlItems.contains(id)) {
        QQuickItem* htmlItem = htmlItems[id];
        htmlItem->setY(offset - htmlItem->height() + lineHeight());
    }


    if(changedLines.contains(id)) {
        if(renderedLines.contains(id)) {
            deleteLineItem(renderedLines[id], id);
            renderedLines.remove(id);
        }
    }
    else if(renderedLines.contains(id)) {
        renderedLines[id]->setY(offset);

        return renderedLines[id];
    }
    else {
        ;//qDebug() << "No" << id << "in cache";
    }

    //qDebug() << "Rendering line" << line.id() << "idx" << line.data();

    QQuickItem* lineItem = createLineItem();
    lineItem->setY(offset);

    if(line.isDoubleWidth()) {
        lineItem->setProperty("doubleWidth", true);
    }
    else if(!lineItem->property("transform").isNull()) {
        lineItem->setProperty("doubleWidth", false);
    }

    int parts = 0;
    qreal x = 0;
    line.render([=,&x,&parts](int from, int to, const ScreenStyle& style) {
        parts++;
        QQuickItem* textSegment = createTextSegment(lineItem);

        QFont f = font();
        if(style.hasFlag(ScreenStyle::Bold))
            f.setBold(true);

        if(line.isDoubleHeightTop() || line.isDoubleHeightBottom())
            f.setPointSizeF(f.pointSizeF()*2);

        textSegment->setProperty("font", f);

        //qDebug() << line.data().mid(from, to - from + 1);

        textSegment->setX(x);

        QColor fgColor = style.fgColor();
        QColor bgColor = style.bgColor();


        if(style.hasFlag(ScreenStyle::DefaultFg))
            fgColor = QColor(255, 255, 255);

        if(style.hasFlag(ScreenStyle::DefaultBg))
            bgColor = QColor(0, 0, 0, 0);

        if(style.hasFlag(ScreenStyle::Inverse)) {
            bgColor.setAlpha(255);
            qSwap(fgColor, bgColor);
        }

        textSegment->setProperty("foregroundColor", fgColor);
        textSegment->setProperty("text", line.data().mid(from, to - from + 1) );
        textSegment->setProperty("backgroundColor", bgColor);

        if(line.isDoubleHeightTop() || line.isDoubleHeightBottom()) {
            textSegment->setProperty("doubleHeight", true);
        }
        else {
            textSegment->setProperty("doubleHeight", false);
        }
        textSegment->setProperty("doubleHeightBottom", line.isDoubleHeightBottom());
//        if(line.isDoubleHeightTop())
  //          QMetaObject::invokeMethod(textSegment, "setDoubleHeightTop");
    //    else if(line.isDoubleHeightBottom())
      //      QMetaObject::invokeMethod(textSegment, "setDoubleHeightBottom");

        x += (to - from + 1)*charWidth();
    });

    //qDebug() << "Rendered" << parts;


    lineItemComponent->completeCreate();

    renderedLines[id] = lineItem;
    changedLines.remove(id);
}

void ExTermItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    if(change == ItemSceneChange) {
        if(window())
            initializeView();
        else
            deinitializeView();
    }
    QQuickItem::itemChange(change, value);
}

void ExTermItem::initializeView()
{
    engine = qmlEngine(this);

    auto screenComponent = new QQmlComponent(engine,QUrl("qrc:/qml/ExTerm/ScreenItem.qml"));
    auto cursorItemComponent = new QQmlComponent(engine,QUrl("qrc:/qml/ExTerm/CursorItem.qml"));
    lineItemComponent = new QQmlComponent(engine,QUrl("qrc:/qml/ExTerm/LineItem.qml"));
    textSegmentComponent = new QQmlComponent(engine,QUrl("qrc:/qml/ExTerm/TextSegment.qml"));
    htmlItemComponent = new QQmlComponent(engine,QUrl("qrc:/qml/ExTerm/HtmlItem.qml"));
//    auto topItemComponent = new QQmlComponent(engine,QUrl("qrc:/qml/ExTerm/TopItem.qml"));


//    auto topItem = qobject_cast<QQuickItem*>(topItemComponent->beginCreate(qmlContext(this)));
//    topItem->setParentItem(this);
//    topItemComponent->completeCreate();

    screenItem = qobject_cast<QQuickItem*>(screenComponent->beginCreate(qmlContext(this)));
    screenItem->setParentItem(this);
    screenComponent->completeCreate();

    cursorItem = qobject_cast<QQuickItem*>(cursorItemComponent->beginCreate(qmlContext(this)));
    cursorItem->setParentItem(this);
    cursorItemComponent->completeCreate();

    fontMetrics = QFontMetricsF(this->font());

    connect(&model, &ScreenModel::lineChanged,
            this, &ExTermItem::onLineChanged);

    connect(&model, &ScreenModel::cursorChanged,
            this, &ExTermItem::onCursorChanged);

    connect(&model, &ScreenModel::onLineFeed,
            this, &ExTermItem::onLineFeed);

    connect(&model, &ScreenModel::cursorBlinkingChanged, [=](bool cursorBlinking) {
        screenItem->setProperty("cursorBlinking", cursorBlinking);
    });
    connect(&model, &ScreenModel::cursorVisibleChanged, [=](bool cursorVisible) {
        screenItem->setProperty("cursorVisible", cursorVisible);
    });


    auto onSizeChanged = [=]() {
        qDebug() << "Size changed height" << lineHeight();
        model.setSize(QSize(width() / charWidth(), height() / lineHeight() /*+1*/));

        renderTimer.start(30);
        scheduleFullRedraw = true;
//        if(engine)
//            renderAll();
        onCursorChanged(model.cursor());
    };
    onSizeChanged();
    connect(this, &QQuickItem::heightChanged, onSizeChanged);
    connect(this, &QQuickItem::widthChanged, onSizeChanged);
    connect(this, &ExTermItem::fontChanged, onSizeChanged);


    renderTimer.setSingleShot(true);
    connect(&renderTimer, &QTimer::timeout, [&]() {

        {
          auto pos = mapCursorToPosition(model.cursor());

          cursorItem->setX(pos.x());
          cursorItem->setY(pos.y());
        }

        if(scheduleFullRedraw) {
            renderAll();
            //scheduleFullRedraw = false;
            return;
        }

        for(auto i = changedLines.begin();i != changedLines.end();) {
            quint64 id = *i;
            int idx = model.getLineIdxById(id);
            if(idx < 0)
                continue;
            auto pos = mapCursorToPosition(QPoint(0, idx));
            ++i;
            renderLine(pos.y(), model[idx]);
        }
        changedLines.clear();
    });

    screenItem->setProperty("cursorVisible", true);
    screenItem->setProperty("cursorBlinking", false);
}

void ExTermItem::deinitializeView()
{
    qDebug() << "Deinitialize view";
    delete screenItem;
    engine = NULL;
}

void ExTermItem::clearRenderedCache()
{
    for(auto i = renderedLines.constBegin();i != renderedLines.constEnd();++i)
        deleteLineItem(i.value(), i.key());
    renderedLines.clear();
    changedLines.clear();
}

QPointF ExTermItem::mapCursorToPosition(const QPoint& cursor)
{
    return QPointF(charWidth() * (qreal)cursor.x(), height() - lineHeight() * (cursor.y() + 1));
}

void ExTermItem::onLineChanged(int idx) {
    changedLines.insert(model[idx].id());

    //auto pos = mapCursorToPosition(QPoint(0, idx));
    //renderLine(pos.y(), model[idx]);
//    scheduleRenderLine(idx);
    if(!renderTimer.isActive())
        renderTimer.start(10);
}

void ExTermItem::onCursorChanged(const QPoint& cursor) {

}

void ExTermItem::onLineFeed() {
    scheduleFullRedraw = true;
    if(!renderTimer.isActive())
        renderTimer.start(50);
}
