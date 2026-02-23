QT += core gui widgets openglwidgets multimedia

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cameracapture.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    FilterContext.h \
    Filters.h \
    VideoRenderWidget.h \
    cameracapture.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Windows Media Foundation 库
# mfplat：Media Foundation 平台mfreadwrite：读写媒体mfuuid：GUID 定义ole32, oleaut32：COM 支持
LIBS += -lmfplat -lmfreadwrite -lmfuuid -lole32 -loleaut32

RESOURCES += \
    res.qrc
