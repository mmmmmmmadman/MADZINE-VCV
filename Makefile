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

# Add Objective-C++ sources for macOS
ifeq ($(shell uname),Darwin)
    SOURCES += $(wildcard src/*.mm)
endif

# Add sst-filters for oversampling (Surge XT technology)
FLAGS += -Isst-filters/include -Isst-basic-blocks/include

DISTRIBUTABLES += res $(wildcard LICENSE*) $(wildcard presets)

include $(RACK_DIR)/plugin.mk

# Use C++20 for sst-filters compatibility
CXXFLAGS := $(filter-out -std=c++11,$(CXXFLAGS)) -std=c++20