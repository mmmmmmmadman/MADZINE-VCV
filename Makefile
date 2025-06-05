# ============================================================================ #
# VCV Rack Plugin Makefile
#
# This Makefile builds your VCV Rack plugin.
# Ensure your plugin.json and source files are in the correct locations.
# ============================================================================ #

# Plugin slug and version from plugin.json (should match)
SLUG = MADZINE
VERSION = 2.0.0

# VCV Rack plugin API version
# For VCV Rack v2 plugins, this MUST be 2.
PLUGIN_VERSION = 2 # <-- 確保沒有 '#' 符號！

# If RACK_DIR is not defined when calling the Makefile, default to two directories above.
# This assumes your Rack-SDK is in ~/Documents/VCV-Dev/Rack-SDK/
# However, it's best practice to always export RACK_DIR manually or in your shell profile.
RACK_DIR ?= ../Rack-SDK # <-- 確認這個路徑是對的，根據你的 Rack-SDK 實際位置

# Custom install directory for your plugin.
# This overrides the default Rack installation path (e.g., ~/Documents/Rack2/plugins).
# Based on your log, this is the correct path for your VCV Rack Pro 2.6.4 macOS ARM64.
INSTALL_DIR = /Users/madzine/Library/Application Support/Rack2/plugins-mac-arm64 # <-- 確保沒有 '#' 符號！

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
# This will automatically find all .cpp files in the 'src' directory.
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
# This line must be at the very end of your Makefile.
include $(RACK_DIR)/plugin.mk