#UwFlashX Qt project qmake file

#Uncomment to have extra malloc failure debugging
#DEFINES += MALLOC_DEBUGGING
#Uncomment to exclude building update check support
#DEFINES += "SKIPUPDATECHECK"
#Uncomment to exclude FTDI-specific bootloader entrance methods
#DEFINES += "SKIPFTDI"
#Uncomment to build console version application (not currently supported)
#DEFINES += "SKIPGUI"

DEFINES += APP_NAME='\\"UwFlashX\\"'

QT       += core serialport

!contains(DEFINES, SKIPGUI) {
QT       += gui
} else {
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UwFlashX
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        LrdFwUpd.cpp \
        LrdFwUART.cpp \
        LrdSettings.cpp \
        LrdFwUwf.cpp \
        LrdErr.cpp \
        LrdFwBlEnter.cpp \
        LrdPopup.cpp

HEADERS += \
        mainwindow.h \
        LrdFwUpd.h \
        LrdFwUART.h \
        LrdFwCommon.h \
        LrdSettings.h \
        LrdFwUwf.h \
        LrdErr.h \
        LrdFwBlEnter.h \
        LrdPopup.h

FORMS += \
        mainwindow.ui \
        LrdPopup.ui

#Application update files and network library
!contains(DEFINES, SKIPUPDATECHECK) {
    QT      += network

    SOURCES += \
        LrdAppUpd.cpp

    HEADERS += \
        LrdAppUpd.h
}

#FTDI-based bootloader entrance options
!contains(DEFINES, SKIPFTDI) {
    #Linux libraries
    unix:!macx: LIBS += -lusb-1.0
    unix:!macx: LIBS += -lftdi1

    #Windows libraries
    !contains(QMAKESPEC, g++) {
        #MSVC build for windows
        contains(QT_ARCH, i386) {
            #32-bit windows
            win32: LIBS += -L$$PWD/FTDI/Win32/ -lftd2xx
        } else {
            #64-bit windows
            win32: LIBS += -L$$PWD/FTDI/Win64/ -lftd2xx
        }

        HEADERS  += FTDI/ftd2xx.h
    }
}

#Windows application version information
win32:RC_FILE = version.rc

#Windows application icon
win32:RC_ICONS = images/UwFlashX32.ico

#Mac application icon
ICON = MacUwFlashXIcon.icns

RESOURCES += \
    UwFlashXImages.qrc
