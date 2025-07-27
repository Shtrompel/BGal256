# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ~/Projects/Rack-SDK

PLUGIN_LDFLAGS += -lglew32 -lmesa
PLUGIN_CFLAGS += -DGL_GLEXT_PROTOTYPES

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -Idep/include -I./src/dep/dr_wav -I./src/dep -I./src -I./src/widgets -I./src/utils
CFLAGS +=
CXXFLAGS += -include stb_image_wrapper.h

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
    # Windows (typically MSYS2/MinGW/Cygwin)
    LDFLAGS += -lopengl32
else
    ifeq ($(UNAME_S),Linux)
        LDFLAGS += -lGL
    else ifeq ($(UNAME_S),Darwin)
        # macOS uses a framework
        LDFLAGS += -framework OpenGL
    endif
endif

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/widgets/*.cpp)
SOURCES += $(wildcard src/utils/*.cpp)
SOURCES += src/dep/dr_wav/dr_wav.cpp


# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
