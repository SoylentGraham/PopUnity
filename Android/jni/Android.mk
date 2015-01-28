LOCAL_PATH := $(call my-dir)
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)


#	build module
include $(CLEAR_VARS)

SOY_DIR = ../$(SOYLENT_DIR)/src
APP_DIR = ../../
# $(SRCROOT)
$(warning $(SOY_DIR))

LOCAL_CPPFLAGS += -D$(subst $(SPACE), -D,$(GCC_PREPROCESSOR_DEFINITIONS))
$(warning $(LOCAL_CPPFLAGS))

LOCAL_C_INCLUDES += $(SOY_DIR)

LOCAL_MODULE    := popunity
LOCAL_SRC_FILES += $(APP_DIR)/src/Android.cpp \
				$(APP_DIR)/src/Unity.cpp \
				$(SOY_DIR)/SoyTypes.cpp
$(warning $(LOCAL_SRC_FILES))

include $(BUILD_SHARED_LIBRARY)
