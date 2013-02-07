#ifndef VT100PARSER_H
#define VT100PARSER_H

#include "parser.h"
#include <QString>
#include <QVector>

class QTextDecoder;
class VT100Parser : public Parser
{
    Q_OBJECT

    QTextDecoder* decoder;

    bool m_cursorKeyMode;
public:
    explicit VT100Parser(ScreenModel* screen);
    ~VT100Parser();

    Q_PROPERTY(bool cursorKeyMode READ cursorKeyMode NOTIFY cursorKeyModeChanged)
    bool cursorKeyMode() const;

signals:
    void cursorKeyModeChanged(bool state);

    void htmlBlock(const QString& objectId, int rows, const QString& data);
    void jsBlock(const QString& targetObjectId, const QString& data);
public slots:
    void onData(const QByteArray& data);

private:
    enum DecodeState {
        PlainText,
        DecodeC0,
        DecodeC1_7bit,
        DecodeCSI,
        DecodeOSC,
        DecodeOtherEscape
    };

    enum DecodeOSCState {
        ChangeWindowAndIconName,
        ChangeIconTitle,
        ChangeWindowTitle,
        ExTermBlock,
        Other, // TODO handle all cases
        None
    };
    DecodeState decodeState;
    DecodeOSCState decodeOscState;
    QString oscData;

    QString currentData;
    int currentTokenStart;
    int currentPosition;
    QChar intermediateChar;
    QString parameterString;
    QVector<int> parameters;

    void decodeCharacter(ushort character);
    void decodeC0(uchar character);
    void decodeC1_7bit(uchar character);
    void decodeParameters(uchar character);
    void decodeCSI(uchar character);
    void decodeOSC(uchar character);
    void decodeOtherEscape(uchar character);
    void tokenFinished();
    void appendParameter();
};

#endif // VT100PARSER_H
