LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES += \
    external/libxml2/include \
    external/icu/icu4c/source/common

LOCAL_SRC_FILES := \
    sensors.cpp \
    SensorBase.cpp \
    LightSensor.cpp \
    ProximitySensor.cpp \
    CompassSensor.cpp \
    Accelerometer.cpp \
    Gyroscope.cpp \
    PressureSensor.cpp \
    InputEventReader.cpp \
    CalibrationManager.cpp \
    NativeSensorManager.cpp \
    VirtualSensor.cpp \
    sensors_XML.cpp

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl libxml2 libutils

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libcalmodule_common

LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

ifdef TARGET_2ND_ARCH
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
else
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libst480
LOCAL_MODULE_OWNER := senodia
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_SRC_FILES := algo/common/st480/lib64/libst480.a
LOCAL_MULTILIB := 64

include $(BUILD_PREBUILT)
