# Offline target: no Qt, hidapi implementation, libusb, or device discovery.
TEMPLATE = app
TARGET = d5next-tests
CONFIG += console c++17 warn_on
CONFIG -= app_bundle qt
QT -= core gui
DEFINES += OPENRGB_D5NEXT_OFFLINE_TESTS

OPENRGB_ROOT = $$clean_path($$PWD/../..)
D5NEXT_DIR = $$OPENRGB_ROOT/Controllers/AquaComputerD5NextController

INCLUDEPATH += $$OPENRGB_ROOT \
               $$OPENRGB_ROOT/RGBController \
               $$OPENRGB_ROOT/dependencies/json \
               $$OPENRGB_ROOT/dependencies/CRCpp \
               $$OPENRGB_ROOT/dependencies/hidapi-win/include \
               $$OPENRGB_ROOT/hidapi_wrapper \
               $$OPENRGB_ROOT/serial_port \
               $$OPENRGB_ROOT/i2c_smbus \
               $$OPENRGB_ROOT/SPDAccessor \
               $$D5NEXT_DIR

SOURCES += $$PWD/D5NextTests.cpp \
           $$PWD/OfflineLog.cpp \
           $$PWD/OfflineDetection.cpp \
           $$D5NEXT_DIR/D5NextProtocol.cpp \
           $$D5NEXT_DIR/AquaComputerD5NextController.cpp \
           $$D5NEXT_DIR/AquaComputerD5NextControllerDetect.cpp \
           $$D5NEXT_DIR/RGBController_AquaComputerD5Next.cpp \
           $$OPENRGB_ROOT/RGBController/RGBController.cpp \
           $$OPENRGB_ROOT/StringUtils.cpp

unix:LIBS += -pthread

contains(CONFIG, d5next_sanitize) {
    QMAKE_CXXFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
    QMAKE_LFLAGS += -fsanitize=address,undefined
}
