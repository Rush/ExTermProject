#include <QGuiApplication>
#include <QtQml>
#include <QQuickView>
#include <extermitem.h>

void registerQmlTypes()
{
    qmlRegisterType<ExTermItem>("pl.rtsolutions.exterm", 1, 0, "ExTermItem");
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
