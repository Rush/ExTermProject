#ifndef PARSER_H
#define PARSER_H

#include <QObject>

class ScreenModel;
class Parser : public QObject
{
    Q_OBJECT
protected:
    ScreenModel& screen;
public:
    explicit Parser(ScreenModel* screen);

signals:

public slots:
    virtual void onData(const QByteArray& data) = 0;
};

#endif // PARSER_H
