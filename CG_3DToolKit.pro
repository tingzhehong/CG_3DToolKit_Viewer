QT       += core gui opengl openglextensions

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CXXFLAGS -= -Zc:strictStrings

DEFINES += SCALAR_TYPE_FLOAT

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/common/ccOptions.cpp \
    src/common/ccOverlayDialog.cpp \
    src/common/ccPickingHub.cpp \
    src/common/ccPluginManager.cpp \
    src/qCC/ccGLWindow.cpp \
    src/qCC/ccPointPickingGenericInterface.cpp \
    src/qCC/ccPointPropertiesDlg.cpp \
    src/ui/CGAboutDialog.cpp \
    src/view/CGConsoleView.cpp

HEADERS += \
    mainwindow.h \
    src/common/ccOptions.h \
    src/common/ccOverlayDialog.h \
    src/common/ccPickingHub.h \
    src/common/ccPluginManager.h \
    src/plugins/ccMainAppInterface.h \
    src/plugins/ccStdPluginInterface.h \
    src/plugins/ccGLPluginInterface.h \
    src/plugins/ccIOPluginInterface.h \
    src/qCC/ccGLWindow.h \
    src/qCC/ccPersistentSettings.h \
    src/qCC/ccPointPickingGenericInterface.h \
    src/qCC/ccPointPropertiesDlg.h \
    src/ui/CGAboutDialog.h \
    src/util/CCCommon.h \
    src/view/CGConsoleView.h

FORMS += \
    mainwindow.ui \
    src/qCC/pointPropertiesDlg.ui

INCLUDEPATH += \
    ./src/common \
    ./src/plugins \
    ./src/qCC \
    ./src/ui \
    ./src/util \
    ./src/view

INCLUDEPATH += \
    $$PWD\lib\CC\include \
    $$PWD\lib\CCFbo\include \
    $$PWD\lib\qCC_db \
    $$PWD\lib\qCC_glWindow \
    $$PWD\lib\qCC_io \
    $$PWD\lib\qcustomplot

LIBS += \
    $$PWD\lib\CC\lib\Release\CC_CORE_LIB.lib \
    $$PWD\lib\CCFbo\lib\Release\CC_FBO_LIB.lib \
    $$PWD\lib\qCC_db\lib\Release\QCC_DB_LIB.lib \
    $$PWD\lib\qCC_glWindow\lib\Release\QCC_GL_LIB.lib \
    $$PWD\lib\qCC_io\lib\Release\QCC_IO_LIB.lib \
    $$PWD\lib\qcustomplot\lib\Release\qcustomplot.lib \

RESOURCES += \
    icones.qrc \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
