# MADZINE VCV Rack Plugin

ifeq ($(ARCH), x64)
    RACK_DIR ?= ../Rack-SDK-x64
    FLAGS += -arch x86_64
    CFLAGS += -arch x86_64
    CXXFLAGS += -arch x86_64 -march=x86-64
    LDFLAGS += -arch x86_64
    override CFLAGS := $(filter-out -march=armv8-a+fp+simd,$(CFLAGS))
    override CXXFLAGS := $(filter-out -march=armv8-a+fp+simd,$(CXXFLAGS))
else
    RACK_DIR ?= ../Rack-SDK
endif

SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res $(wildcard LICENSE*) $(wildcard presets)

include $(RACK_DIR)/plugin.mk