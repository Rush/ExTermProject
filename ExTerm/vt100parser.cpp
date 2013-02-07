#include "vt100parser.h"

#include <QTextCodec>

#include <QDebug>

#include "controlchars.h"

#include "screenmodel.h"

#include <QColor>

static QColor brightPalette8[8] = {
    QColor(49, 49, 49), // gray
    QColor(255, 84, 84), // bright red
    QColor(84, 255, 84), // bright green
    QColor(255, 255, 84), // bright yellow
    QColor(84, 84, 255), // bright blue
    QColor(255, 84, 255), // bright pink
    QColor(84, 255, 255), // bright cyan
    QColor(255, 255, 255) // bright white
};


static QColor darkPalette8[8] = {
    QColor(0, 0, 0), // black
    QColor(178, 24, 24), // dark red
    QColor(24, 178, 24), // dark green
    QColor(178, 178, 24), // dark yellow
    QColor(24, 24, 178), // dark blue
    QColor(178, 24, 178), // dark pink
    QColor(24, 178, 178), // dark cyan
    QColor(178, 178, 178) // dark white
};

static QColor color256(quint8 index)
{
    // first 16 are the base colours
    if(index < 8) {
        return darkPalette8[index];
    }
    else if(index < 16) {
        return brightPalette8[index-8];
    }
    index -= 16;

    //  16 to 231 is a 6x6x6 rgb colour cube
    if (index < 216) {
        int r = 0, g = 0, b = 0;
        if(index / 36 % 6)
            r = 40 * ((index / 36) % 6) + 55;
        if((index / 6) % 6)
            g = 40 * ((index / 6) % 6) + 55;
        if((index / 1) % 6)
            b = 40 * ((index / 1) % 6) + 55;
        return QColor(r, g, b);
    }
    index -= 216;

    // 232 to 255 is grayscale without the base black and white
    int gray = index * 10 + 8;
    return QColor(gray, gray, gray);
}


VT100Parser::VT100Parser(ScreenModel* screen) :
    Parser(screen)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    decoder = codec->makeDecoder();

    decodeState = PlainText;
    currentTokenStart = 0;
    currentPosition = 0;
    intermediateChar = QChar();
    m_cursorKeyMode = 0;
}

void VT100Parser::onData(const QByteArray &rawData)
{
    currentData = decoder->toUnicode(rawData.data(), rawData.size());
    if(currentData.length() == 0)
        return;

    currentTokenStart = 0;
    for (currentPosition = 0; currentPosition < currentData.length(); currentPosition++) {

        QChar qChar = currentData.at(currentPosition);

        decodeCharacter(qChar.unicode());
    }
    if (decodeState == PlainText) {
        QString text = currentData.mid(currentTokenStart);
        if (text.size()) {
            screen.insertAtCursor(QString(text));
            tokenFinished();
        }
    }

    currentData = "";
}

VT100Parser::~VT100Parser()
{
    delete decoder;
}

bool VT100Parser::cursorKeyMode() const
{
    return m_cursorKeyMode;
}

void VT100Parser::decodeCharacter(ushort character)
{
    switch (decodeState) {
    case PlainText:
        if (character < C0::C0_END ||
                (character >= C1_8bit::C1_8bit_Start &&
                 character <= C1_8bit::C1_8bit_Stop)) {

            if (currentPosition != currentTokenStart) {
                screen.insertAtCursor(QString(currentData.mid(currentTokenStart,
                                          currentPosition - currentTokenStart)));
                tokenFinished();
                currentTokenStart--;
            }
            decodeState = DecodeC0;
            decodeC0(character);
        }
        break;
    case DecodeC0:
        decodeC0(character);
        break;
    case DecodeC1_7bit:
        decodeC1_7bit(character);
        break;
    case DecodeCSI:
        decodeCSI(character);
        break;
    case DecodeOSC:
        decodeOSC(character);
        break;
    case DecodeOtherEscape:
        decodeOtherEscape(character);
        break;
   }
}

void VT100Parser::decodeC0(uchar character)
{
    switch (character) {
    case C0::NUL:
    case C0::SOH:
    case C0::STX:
    case C0::ETX:
    case C0::EOT:
    case C0::ENQ:
    case C0::ACK:
        qDebug() << "Unhandled Controll character" << character;
        tokenFinished();
        break;
    case C0::BEL:
        //m_screen->scheduleFlash();
        //qDebug() << "Unimplemented bell";
        tokenFinished();
        break;
    case C0::BS:
        //qDebug() << "backspace";
        screen.backspace();
        tokenFinished();
        break;
    case C0::HT:
    {
        int x = screen.cursor().x();
        int spaces = 8 - (x % 8);
        screen.insertAtCursor(QString(spaces,' '));

        tokenFinished();
        break;
    }
    case C0::LF:
        screen.lineFeed();
        tokenFinished();
        break;
    case C0::VT:
    case C0::FF:
        qDebug() << "Unhandled Controll character" << character;
        tokenFinished();
        break;
    case C0::CR:
        screen.moveCursorHome();
        tokenFinished();
        //next should be a linefeed;
        break;
    case C0::SOorLS1:
    case C0::SIorLS0:
    case C0::DLE:
    case C0::DC1:
    case C0::DC2:
    case C0::DC3:
    case C0::DC4:
    case C0::NAK:
    case C0::SYN:
    case C0::ETB:
    case C0::CAN:
    case C0::EM:
    case C0::SUB:
        qDebug() << "Unhandled Controll character" << character;
        tokenFinished();
        break;
    case C0::ESC:
        decodeState = DecodeC1_7bit;
        break;
    case C0::IS4:
    case C0::IS3:
    case C0::IS2:
    case C0::IS1:
    default:
        qDebug() << "Unhandled Controll character" << character;
        tokenFinished();
        break;
    }
}

void VT100Parser::decodeC1_7bit(uchar character)
{
    switch(character) {
    case C1_7bit::CSI:
        decodeState = DecodeCSI;
        break;
    case C1_7bit::OSC:
        decodeState = DecodeOSC;
        break;
    case C1_7bit::RI:
        qDebug() << "Unimplemented reverse line feed";
        //m_screen->reverseLineFeed();
        tokenFinished();
        break;
    case '%':
    case '#':
    case '(':
        parameters.append(-character);
        decodeState = DecodeOtherEscape;
        break;
    case '=':
        //qDebug() << "Application keypad";
        tokenFinished();
        break;
    case '>':
        //qDebug() << "Normal keypad mode";
        tokenFinished();
        break;
    default:
        qDebug() << "Unhandled C1_7bit character" << character;
        tokenFinished();
    }
}

void VT100Parser::decodeParameters(uchar character)
{
    switch (character) {
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        parameterString.append(character);
        break;
    case 0x3a:
        qDebug() << "Encountered special delimiter in parameterbyte";
        break;
    case 0x3b:
        appendParameter();
        break;
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
        appendParameter();
        parameters.append(-character);
        break;
    default:
        //this is undefined for now
        qDebug() << "Encountered undefined parameter byte";
        break;
    }
}

void VT100Parser::decodeCSI(uchar character)
{
        if (character >= 0x30 && character <= 0x3f) {
            decodeParameters(character);
        } else {
            if (character >= 0x20 && character <= 0x2f) {
                if (intermediateChar.unicode())
                    qDebug() << "Warning!: double intermediate bytes found in CSI";
                intermediateChar = character;
            } else if (character >= 0x40 && character <= 0x7d) {
                if (intermediateChar.unicode()) {
                    switch (character) {
                    case FinalBytesSingleIntermediate::SL:
                    case FinalBytesSingleIntermediate::SR:
                    case FinalBytesSingleIntermediate::GSM:
                    case FinalBytesSingleIntermediate::GSS:
                    case FinalBytesSingleIntermediate::FNT:
                    case FinalBytesSingleIntermediate::TSS:
                    case FinalBytesSingleIntermediate::JFY:
                    case FinalBytesSingleIntermediate::SPI:
                    case FinalBytesSingleIntermediate::QUAD:
                    case FinalBytesSingleIntermediate::SSU:
                    case FinalBytesSingleIntermediate::PFS:
                    case FinalBytesSingleIntermediate::SHS:
                    case FinalBytesSingleIntermediate::SVS:
                    case FinalBytesSingleIntermediate::IGS:
                    case FinalBytesSingleIntermediate::IDCS:
                    case FinalBytesSingleIntermediate::PPA:
                    case FinalBytesSingleIntermediate::PPR:
                    case FinalBytesSingleIntermediate::PPB:
                    case FinalBytesSingleIntermediate::SPD:
                    case FinalBytesSingleIntermediate::DTA:
                    case FinalBytesSingleIntermediate::SHL:
                    case FinalBytesSingleIntermediate::SLL:
                    case FinalBytesSingleIntermediate::FNK:
                    case FinalBytesSingleIntermediate::SPQR:
                    case FinalBytesSingleIntermediate::SEF:
                    case FinalBytesSingleIntermediate::PEC:
                    case FinalBytesSingleIntermediate::SSW:
                    case FinalBytesSingleIntermediate::SACS:
                    case FinalBytesSingleIntermediate::SAPV:
                    case FinalBytesSingleIntermediate::STAB:
                    case FinalBytesSingleIntermediate::GCC:
                    case FinalBytesSingleIntermediate::TATE:
                    case FinalBytesSingleIntermediate::TALE:
                    case FinalBytesSingleIntermediate::TAC:
                    case FinalBytesSingleIntermediate::TCC:
                    case FinalBytesSingleIntermediate::TSR:
                    case FinalBytesSingleIntermediate::SCO:
                    case FinalBytesSingleIntermediate::SRCS:
                    case FinalBytesSingleIntermediate::SCS:
                    case FinalBytesSingleIntermediate::SLS:
                    case FinalBytesSingleIntermediate::SCP:
                    default:
                        qDebug() << "unhandled CSI FinalBytesSingleIntermediate sequence" << character;
                        tokenFinished();
                        break;
                    }
                } else {
                    switch (character) {
                    case FinalBytesNoIntermediate::ICH:
                        qDebug() << "ICH" << parameters;
                        tokenFinished();
                        //qDebug() << parameters[0];
                        //qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        break;
                    case FinalBytesNoIntermediate::CUU: // Cursor Up PS Times
                        appendParameter();
                        if(parameters.size())
                            screen.moveCursorUp(parameters[0]);
                        else
                            screen.moveCursorUp();
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::CUD:
                        tokenFinished();
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        break;
                    case FinalBytesNoIntermediate::CUF:{
                        //qDebug() << "CUF";
                        appendParameter();
                        Q_ASSERT(parameters.size() < 2);
                        int count = parameters.size() ? parameters.at(0) : 1;
                        screen.moveCursorRight(count);
                        tokenFinished();
                    }
                        break;
                    case FinalBytesNoIntermediate::CUB:
                    case FinalBytesNoIntermediate::CNL:
                    case FinalBytesNoIntermediate::CPL:
                    case FinalBytesNoIntermediate::CHA:
                        tokenFinished();
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        break;
                    case FinalBytesNoIntermediate::CUP:
                        appendParameter();
                        if (!parameters.size()) {
                            //screen.moveCursorUp();
                            //screen.moveCursorHome();
                            screen.moveCursorTo(QPoint(1, 1));
                        }
                        else if (parameters.size() == 2) {
                            screen.moveCursorTo(QPoint(parameters[1], parameters[0]));
                        }
                        else {
                            qDebug() << "CUP bad parameters" << parameters;
                        }


                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::CHT:
                        tokenFinished();
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        break;
                    case FinalBytesNoIntermediate::ED:
//                        qDebug() << "ED unimplemented";
                        appendParameter();
                        if (!parameters.size()) {
                            //m_screen->eraseFromCurrentLineToEndOfScreen();
                            screen.clearToEndOfScreen();
                        } else {
                            switch (parameters.at(0)) {
                            case 1:
                                //m_screen->eraseFromCurrentLineToBeginningOfScreen();

                                screen.clearToBeginningOfScreen();
                                break;
                            case 2:
                                screen.clearScreen();
                                break;
                            default:
                                qDebug() << "Invalid parameter value for FinalBytesNoIntermediate::ED";
                            }
                        }

                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::EL:
                        appendParameter();
                        if (!parameters.size() || parameters.at(0) == 0) {
                            screen.eraseFromCursor();
                        } else if (parameters.at(0) == 1) {
                            qDebug() << "Unimplemented erase cursor to position";
                            //m_screen->eraseToCursorPosition();
                        } else if (parameters.at(0) == 2) {
                            qDebug() << "Unimplemented eraseLine";
                            //m_screen->eraseLine();
                        } else{
                            qDebug() << "Fault when processing FinalBytesNoIntermediate::EL";
                        }
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::IL: {
                        appendParameter();
                        int count = 1;
                        if (parameters.size()) {
                            count = parameters.at(0);
                        }
                        qDebug() << "Unimplemented insertLines";
                        //m_screen->insertLines(count);
                        tokenFinished();
                    }
                        break;
                    case FinalBytesNoIntermediate::DL: {
                        appendParameter();
                        int count = 1;
                        if (parameters.size()) {
                            count = parameters.at(0);
                        }
                        qDebug() << "Unimplemented deleteLines";
                        //m_screen->deleteLines(count);
                        tokenFinished();
                    }
                        break;
                    case FinalBytesNoIntermediate::EF:
                    case FinalBytesNoIntermediate::EA:
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::DCH:
                    {
                        appendParameter();
                        int count = 1;
                        if(parameters.size())
                            count = parameters[0];
                        screen.deleteRight(count);
                        tokenFinished();
                        break;
                    }
                    case FinalBytesNoIntermediate::SSE:
                    case FinalBytesNoIntermediate::CPR:
                    case FinalBytesNoIntermediate::SU:
                    case FinalBytesNoIntermediate::SD:
                    case FinalBytesNoIntermediate::NP:
                    case FinalBytesNoIntermediate::PP:
                    case FinalBytesNoIntermediate::CTC:
                    case FinalBytesNoIntermediate::ECH:
                    case FinalBytesNoIntermediate::CVT:
                    case FinalBytesNoIntermediate::CBT:
                    case FinalBytesNoIntermediate::SRS:
                    case FinalBytesNoIntermediate::PTX:
                    case FinalBytesNoIntermediate::SDS:
                    case FinalBytesNoIntermediate::SIMD:
                    case FinalBytesNoIntermediate::HPA:
                    case FinalBytesNoIntermediate::HPR:
                    case FinalBytesNoIntermediate::REP:
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::DA:
                        appendParameter();
                        if (parameters.size()) {
                            switch (parameters.at(0)) {
                            case -'>':
                                //m_screen->sendSecondaryDA();
                                break;
                            case -'?':
                                qDebug() << "WHAT!!!";
                                break; //ignore
                            case 0:
                            default:
                                ;//m_screen->sendPrimaryDA();
                            }
                        } else {
                            //m_screen->sendPrimaryDA();
                        }
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::VPA:
                    case FinalBytesNoIntermediate::VPR:
                    case FinalBytesNoIntermediate::HVP:
                    case FinalBytesNoIntermediate::TBC:
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::SM:
                        appendParameter();
                        if (parameters.size() && parameters.at(0) == -'?') {
                            if (parameters.size() > 1) {
                                switch (parameters.at(1)) {
                                case 1:
                                    m_cursorKeyMode =  true;
                                    emit cursorKeyModeChanged(true);
                                    break;
                                case 12:
                                    screen.setCursorBlinking(true);
                                    break;
                                case 25:
                                    screen.setCursorVisible(true);
                                    break;
                                case 1034:
                                    //I don't know what this sequence is
                                    break;
                                case 1049:
                                    qDebug() << "Unimplemented save cursor and save screen data";
                                    //m_screen->saveCursor();
                                    //m_screen->saveScreenData();
                                    break;
                                default:
                                    qDebug() << "unhandled CSI FinalBytesNoIntermediate::SM ? with parameter:" << parameters.at(1);
                                }
                            } else {
                                qDebug() << "unhandled CSI FinalBytesNoIntermediate::SM ?";
                            }
                        } else {
                            qDebug() << "unhandled CSI FinalBytesNoIntermediate::SM";
                        }
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::MC:
                    case FinalBytesNoIntermediate::HPB:
                    case FinalBytesNoIntermediate::VPB:
                        qDebug() << "unhandled CSI FinalBytesNoIntermediate sequence" << character;
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::RM:
                        appendParameter();
                        if (parameters.size()) {
                            qDebug() << parameters;
                            switch(parameters.at(0)) {
                            case -'?':
                                if (parameters.size() > 1) {
                                    switch(parameters.at(1)) {
                                    case 1:
                                        m_cursorKeyMode =  false;
                                        emit cursorKeyModeChanged(true);
                                        break;
                                    case 12:
                                        screen.setCursorBlinking(false);
                                        break;
                                    case 25:
                                        screen.setCursorVisible(false);
                                        break;
                                    case 1049:
                                        qDebug() << "Unimplemented restore cursor and restore screen data";
                                        //m_screen->restoreCursor();
                                        //m_screen->restoreScreenData();
                                        break;
                                    default:
                                        qDebug() << "unhandled CSI FinalBytesNoIntermediate::RM? with "
                                                    "parameter " << parameters.at(1);
                                    }
                                } else {
                                    qDebug() << "unhandled CSI FinalBytesNoIntermediate::RM";
                                }
                                break;
                            case 4:
                                qDebug() << "REPLACE MODE!";
                            default:
                                qDebug() << "unhandled CSI FinalBytesNoIntermediate::RM";
                                break;
                            }
                        }
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::SGR: {
                        appendParameter();

                        if (!parameters.size())
                            parameters << 0;

                        for (int i = 0; i < parameters.size();i++) {
                            switch(parameters.at(i)) {
                            case 0:
                                screen.style().reset();
                                break;
                            case 1:
                                screen.style().setFlag(ScreenStyle::Bold);
                                break;
                            case 5:
                                screen.style().setFlag(ScreenStyle::Blinking);
                                break;
                            case 4:
                                qDebug() << "Set palette!!";
                                tokenFinished();
                                break;
                            case 7:
                                screen.style().setFlag(ScreenStyle::Inverse);
                                break;
                            case 8:
                            {
                                QColor c = screen.style().fgColor();
                                c.setAlpha(0);
                                screen.style().setFgColor(c);
                                break;
                            }
                            case 22:
                                screen.style().clearFlag(ScreenStyle::Bold);
                                break;
                            case 24:
                                screen.style().clearFlag(ScreenStyle::Underline);
                                break;
                            case 25:
                                screen.style().clearFlag(ScreenStyle::Blinking);
                                break;
                            case 27:
                                screen.style().clearFlag(ScreenStyle::Inverse);
                                break;
                            case 28:
                            {
                                QColor c = screen.style().fgColor();
                                c.setAlpha(255);
                                screen.style().setFgColor(c);
                                break;
                            }
                            case 39:
                                screen.style().setFlag(ScreenStyle::DefaultFg);
                                break;
                            case 49:
                                screen.style().setFlag(ScreenStyle::DefaultBg);
                                break;
                            case 30:
                            case 31:
                            case 32:
                            case 33:
                            case 34:
                            case 35:
                            case 36:
                            case 37:
                                //screen.setFgColor(palette[parameters[i] - 30]);
                                //qDebug() << "Set fg color";
                                if(screen.style().hasFlag(ScreenStyle::Bold))
                                    screen.style().setFgColor(brightPalette8[parameters[i] - 30]);
                                else
                                    screen.style().setFgColor(darkPalette8[parameters[i] - 30]);
                                break;
                            case 40:
                            case 41:
                            case 42:
                            case 43:
                            case 44:
                            case 45:
                            case 46:
                            case 47:
                                //m_screen->setTextStyleColor(parameters.at(i));
                                //qDebug() << "Set bg color" << parameters[i];
                                screen.style().setBgColor(darkPalette8[parameters[i] - 40]);
                                break;
                            // below is 16-colors xterm support - http://www.xfree86.org/current/ctlseqs.html
                            case 90:
                            case 91:
                            case 92:
                            case 93:
                            case 94:
                            case 95:
                            case 96:
                            case 97:
                                screen.style().setFgColor(brightPalette8[parameters[i] - 90]);
                                break;
                            case 100:
                            case 101:
                            case 102:
                            case 103:
                            case 104:
                            case 105:
                            case 106:
                            case 107:
                                screen.style().setBgColor(brightPalette8[parameters[i] - 100]);
                                break;
                            case 38:
                                appendParameter();
                                qDebug() << "256 colors support fg" << parameters;
                                screen.style().setFgColor(color256(parameters[2]));
                                i = parameters.size();
                                tokenFinished();
                                break;
                            case 48:
                                appendParameter();
                                screen.style().setBgColor(color256(parameters[2]));
                                i = parameters.size();
                                tokenFinished();
                                break;
                            default:
                                qDebug() << "Unknown SGR" << parameters.at(i);
                            }
                        }

                        tokenFinished();
                    }
                        break;
                    case FinalBytesNoIntermediate::DSR:
                        qDebug() << "report";
                    case FinalBytesNoIntermediate::DAQ:
                    case FinalBytesNoIntermediate::Reserved0:
                    case FinalBytesNoIntermediate::Reserved1:
                        qDebug() << "Unhandeled CSI FinalBytesNoIntermediate squence" << character;
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::Reserved2:
                        appendParameter();
                        if (parameters.size() == 2) {
                            if (parameters.at(0) >= 0) {
                                qDebug() << "Unimplemented setScrollArea";
                                //m_screen->setScrollArea(parameters.at(0),parameters.at(1));
                            } else {
                                qDebug() << "Unknown value for scrollRegion";
                            }
                        } else {
                            qDebug() << "Unknown parameterset for scrollRegion";
                        }
                        tokenFinished();
                        break;
                    case FinalBytesNoIntermediate::Reserved3:
                    case FinalBytesNoIntermediate::Reserved4:
                    case FinalBytesNoIntermediate::Reserved5:
                    case FinalBytesNoIntermediate::Reserved6:
                    case FinalBytesNoIntermediate::Reserved7:
                    case FinalBytesNoIntermediate::Reserved8:
                    case FinalBytesNoIntermediate::Reserved9:
                    case FinalBytesNoIntermediate::Reserveda:
                    case FinalBytesNoIntermediate::Reservedb:
                    case FinalBytesNoIntermediate::Reservedc:
                    case FinalBytesNoIntermediate::Reservedd:
                    case FinalBytesNoIntermediate::Reservede:
                    case FinalBytesNoIntermediate::Reservedf:
                    default:
                        qDebug() << "Unhandeled CSI FinalBytesNoIntermediate squence" << character;
                        tokenFinished();
                        break;
                    }
                }
            }
        }
}

void VT100Parser::decodeOSC(uchar character)
{

    if (!parameters.size() &&
            character >= 0x30 && character <= 0x3f) {

        decodeParameters(character);
    }
    else
    {
        if (decodeOscState ==  None) {
            appendParameter();
            if (parameters.size() != 1) {
                tokenFinished();
                return;
            }
            oscData.clear();
            switch (parameters.at(0)) {
            case 0:
                decodeOscState = ChangeWindowAndIconName;
                break;
            case 1:
                decodeOscState = ChangeIconTitle;
                break;
            case 2:
                decodeOscState = ChangeWindowTitle;
                break;
            case 4:
                // TODO: set 256-color palette
                decodeOscState = Other;
                break;
            case 105:
                decodeOscState = ExTermBlock;
                break;
            default:
                qDebug() << parameters;

                decodeOscState = Other;
            }
            oscData.append(QChar(character));
        }
        else if (character == 0x07 || (decodeOscState == Other && character == 0x5c)) {
            if (decodeOscState == ChangeWindowAndIconName || decodeOscState == ChangeWindowTitle) {
                //m_screen->setTitle(title);
            }
            else if(decodeOscState == ExTermBlock) {
                int index = oscData.indexOf(QChar(';'));
                QStringRef type = oscData.leftRef(index);
                QStringRef data = oscData.midRef(index+1);
                if(type.compare("HTML") == 0) {
                    int argIndexes[2] = {0,};
                    int argN = 0;
                    for(int i = 0;i < data.length();++i) {
                        if(data.at(i) == QChar(';')) {
                            argIndexes[argN++] = i;
                        }
                        if(argN == 2)
                            break;
                    }
                    QStringRef objectId = oscData.midRef(index + 1, argIndexes[0]);
                    QStringRef numLines = oscData.midRef(index + 1 + argIndexes[0] + 1,  argIndexes[1] - argIndexes[0] - 1);
                    QStringRef actualData = oscData.midRef(index + 1 + argIndexes[1] + 1);
                    int lines = numLines.toString().toInt();
                    for(int i = 0;i < lines-1;++i)
                        screen.lineFeed();
                    emit htmlBlock(objectId.toString(), lines, actualData.toString());
                }
                else if(type.compare("JS")) {

                }
            }
            tokenFinished();
        }
        else {
            oscData.append(QChar(character));
        }
    }
}

void VT100Parser::decodeOtherEscape(uchar character)
{
    Q_ASSERT(parameters.size());
    switch(parameters.at(0)) {
    case -'(':
        switch(character) {
        case 0:
            //m_screen->setCharacterMap("DEC Special Character and Line Drawing Set");
            break;
        case 'A':
            //m_screen->setCharacterMap("UK");
            break;
        case 'B':
            //m_screen->setCharacterMap("USASCII");
            break;
        case '4':
            //m_screen->setCharacterMap("Dutch");
            break;
        case 'C':
        case '5':
            //m_screen->setCharacterMap("Finnish");
            break;
        case 'R':
            //m_screen->setCharacterMap("French");
            break;
        case 'Q':
            //m_screen->setCharacterMap("FrenchCanadian");
            break;
        case 'K':
            //m_screen->setCharacterMap("German");
            break;
        case 'Y':
            //m_screen->setCharacterMap("Italian");
            break;
        case 'E':
        case '6':
            //m_screen->setCharacterMap("NorDan");
            break;
        case 'Z':
            //m_screen->setCharacterMap("Spanish");
            break;
        case 'H':
        case '7':
            //m_screen->setCharacterMap("Sweedish");
            break;
        case '=':
            //m_screen->setCharacterMap("Swiss");
            break;
        default:
            qDebug() << "Not supported Character set!";
        }
        break;
    case -'#':
        switch(character) {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
            screen.eraseFromCursor();
            screen.setLineMode((ScreenLine::LineMode)(QString(character).toInt()));
            break;
        }
        break;
    default:
        qDebug() << "Other Escape sequence not recognized";
    }
    tokenFinished();
}

void VT100Parser::tokenFinished()
{
    decodeState = PlainText;
    decodeOscState = None;

    parameters.clear();
    parameterString.clear();

    currentTokenStart = currentPosition + 1;
    intermediateChar = 0;
}

void VT100Parser::appendParameter()
{
    if (parameterString.size()) {
        parameters.append(parameterString.toUShort());
        parameterString.clear();
    }
}

