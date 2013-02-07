# Add more folders to ship with the application, here
folder_01.source = qml/ExTerm
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

QT += gui quick webkit webkitwidgets

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

HEADERS += \
    ExTermItem.h

HEADERS += \
    extermitem.h

SOURCES += \
    extermitem.cpp

OTHER_FILES += \
    qml/ExTerm/main.qml

RESOURCES += \
    qml.qrc

HEADERS += \
    basepty.h

SOURCES += \
    basepty.cpp

HEADERS += \
    unixpty.h

SOURCES += \
    unixpty.cpp

unix: LIBS += -lutil

unix: QMAKE_CXXFLAGS += -std=c++0x

HEADERS += \
    parser.h

SOURCES += \
    parser.cpp

HEADERS += \
    screenmodel.h

SOURCES += \
    screenmodel.cpp

HEADERS += \
    vt100parser.h

SOURCES += \
    vt100parser.cpp

OTHER_FILES += \
    qml/ExTerm/TerminalScreen.qml

OTHER_FILES += \
    qml/ExTerm/ExTermScreen.qml

OTHER_FILES += \
    qml/ExTerm/ScreenItem.qml

OTHER_FILES += \
    qml/ExTerm/TextSegment.qml

OTHER_FILES += \
    qml/ExTerm/LineItem.qml

HEADERS += \
    controlchars.h

OTHER_FILES += \
    qml/ExTerm/WebItem.qml

OTHER_FILES += \
    qml/ExTerm/CursorItem.qml

OTHER_FILES += \
    qml/ExTerm/HtmlItem.qml

RESOURCES += \
    html.qrc

OTHER_FILES += \
    qml/ExTerm/TopItem.qml
