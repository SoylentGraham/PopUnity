#LOCAL_PATH := $(call my-dir)
LOCAL_PATH := $(SRCROOT)
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

#	build module
include $(CLEAR_VARS)

# split list of preprocessors by space, and join again with -D
LOCAL_CPPFLAGS += -D$(subst $(SPACE),$(SPACE)-D,$(GCC_PREPROCESSOR_DEFINITIONS))


# c++11 support
# Instruct to use the static GNU STL implementation
# http://stackoverflow.com/questions/4893403/cant-include-c-headers-like-vector-in-android-ndk
LOCAL_CPPFLAGS += $(ANDROID_CPP_FLAGS)
LOCAL_C_INCLUDES += ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.8/include
#$(warning ndk:$(ANDROID_NDK))


# include dirs
#	remove /** (recursive) from header paths
HEADER_PATHS := $(subst /**,$(SPACE),$(HEADER_SEARCH_PATHS))
LOCAL_C_INCLUDES += $(HEADER_PATHS)

LOCAL_MODULE    := $(ANDROID_MODULE)
LOCAL_SRC_FILES += src/Android.cpp
LOCAL_SRC_FILES += $(SOYLENT_DIR)/src/SoyTypes.cpp
#LOCAL_SRC_FILES += src/Unity.cpp

$(warning $(LOCAL_SRC_FILES))

include $(BUILD_SHARED_LIBRARY)
