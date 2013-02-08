#include <QGuiApplication>
#include <QtQml>
#include <QQuickView>
#include <extermitem.h>
#include <extermio.h>

void registerQmlTypes()
{
    qmlRegisterType<ExTermItem>("pl.rtsolutions.exterm", 1, 0, "ExTermItem");
    qmlRegisterType<ExTermIO>("ExTermIO", 1, 0, "ExTermIO");
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    registerQmlTypes();
    QQuickView view(QUrl("qrc:/qml/ExTerm/main.qml"));

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();

    return app.exec();
}
