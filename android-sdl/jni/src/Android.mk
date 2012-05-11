LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_CFLAGS := -g -DANDROID $(LOCAL_CFLAGS)

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# Add your application source files here...
LOCAL_SRC_FILES := my_app.c

LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -ldl -lGLESv2  -llog

include $(BUILD_SHARED_LIBRARY)
