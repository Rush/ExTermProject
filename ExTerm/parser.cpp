#include "parser.h"
#include "screenmodel.h"

Parser::Parser(ScreenModel* screenPtr) :
    QObject(screenPtr), screen(*screenPtr)
{

}
