# Offline target: no Qt, hidapi implementation, libusb, or device discovery.
TEMPLATE = app
TARGET = d5next-tests
CONFIG += console c++17 warn_on
CONFIG -= app_bundle qt
QT -= core gui
contains(CONFIG, d5next_detection) {
    TARGET = d5next-detection-tests
    DEFINES += OPENRGB_D5NEXT_READ_ONLY_DETECTION
    SOURCES += $$PWD/DetectionTests.cpp $$PWD/OfflineDetection.cpp
    unix:LIBS += -Wl,--wrap=access
} else:contains(CONFIG, d5next_production) {
    TARGET = d5next-production-tests
    SOURCES += $$PWD/ProductionTests.cpp
} else {
    DEFINES += OPENRGB_D5NEXT_OFFLINE_TESTS
    SOURCES += $$PWD/D5NextTests.cpp $$PWD/OfflineDetection.cpp
}

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

SOURCES += $$PWD/OfflineLog.cpp \
           $$D5NEXT_DIR/D5NextProtocol.cpp \
           $$D5NEXT_DIR/D5NextRGBpx.cpp \
           $$D5NEXT_DIR/D5NextEffects.cpp \
           $$D5NEXT_DIR/D5NextSystemCommands.cpp \
           $$D5NEXT_DIR/D5NextOwnership.cpp \
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
