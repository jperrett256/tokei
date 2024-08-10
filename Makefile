BUILD_DIR := build
BUILD_MISC_DIR := $(BUILD_DIR)/_misc
INC_DIRS := inc

SRC_FILES := src/main.cpp

COMP_OPTS := /std:c++17 /EHsc /utf-8
# NOTE d3dcompiler.lib only needed if compiling shaders at runtime
# NOTE dxgi.lib and dxguid.lib only needed for DXGI debug breakpoints
LINK_OPTS = /link user32.lib d3d11.lib # d3dcompiler.lib dxgi.lib dxguid.lib # winmm.lib  gdi32.lib

COMP_OPTS_DEBUG := $(COMP_OPTS) /Z7 /Od /Oi /DJDP_DEBUG /DJDP_DEBUG_DX11
COMP_OPTS_RELEASE := $(COMP_OPTS) /O2

EXE_FILE := main.exe

default: msvc-debug


IMGUI_BUILD_DIR := $(BUILD_MISC_DIR)

IMGUI_SRC_BASE_DIR := third_party/imgui/imgui-1.90-docking
IMGUI_SRC_BACKENDS_DIR := $(IMGUI_SRC_BASE_DIR)/backends

IMGUI_SRC_FILES := $(wildcard $(IMGUI_SRC_BASE_DIR)/*.cpp)
IMGUI_SRC_FILES += $(IMGUI_SRC_BACKENDS_DIR)/imgui_impl_win32.cpp $(IMGUI_SRC_BACKENDS_DIR)/imgui_impl_dx11.cpp

IMGUI_INC_DIRS := $(IMGUI_SRC_BASE_DIR) $(IMGUI_SRC_BACKENDS_DIR)

IMGUI_OBJ_FILES := $(patsubst %.cpp,$(IMGUI_BUILD_DIR)/%.obj,$(notdir $(IMGUI_SRC_FILES)))

IMGUI_INC_FLAGS := $(patsubst %,/I%,$(IMGUI_INC_DIRS))

IMGUI_COMP_OPTS := /c /std:c++17 /EHsc /nologo /WX /W4 /Ox $(IMGUI_INC_FLAGS)
IMGUI_COMP_OPTS += /Fo$(IMGUI_BUILD_DIR)/ /Fd$(IMGUI_BUILD_DIR)/ /Fe$(IMGUI_BUILD_DIR)/

$(IMGUI_OBJ_FILES): $(IMGUI_BUILD_DIR)
	cl $(IMGUI_COMP_OPTS) $(IMGUI_SRC_FILES)

# TODO should all obj/pdb files (not just cimgui objs) be placed in a separate build/_misc folder?

INC_DIRS += $(IMGUI_INC_DIRS)


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_MISC_DIR): $(BUILD_DIR)
	mkdir -p $(BUILD_MISC_DIR)

clean:
	rm -r $(BUILD_DIR)

run:
	$(BUILD_DIR)/$(EXE_FILE)


MSVC_INC_FLAGS := $(patsubst %,/I%,$(INC_DIRS))

MSVC_COMP_OPTS := /Fo$(BUILD_MISC_DIR)/ /Fd$(BUILD_MISC_DIR)/ $(MSVC_INC_FLAGS) /nologo /WX /W4 /wd4100 /DJDP_COMPILER_CL
MSVC_COMP_OPTS_MAIN := $(MSVC_COMP_OPTS) /Fe$(BUILD_DIR)/$(EXE_FILE)

MSVC_COMP_OPTS_DEBUG := $(MSVC_COMP_OPTS_MAIN) $(COMP_OPTS_DEBUG)
MSVC_COMP_OPTS_RELEASE := $(MSVC_COMP_OPTS_MAIN) $(COMP_OPTS_RELEASE)

MSVC_LINK_OPTS = $(LINK_OPTS) /subsystem:windows

msvc-debug: $(IMGUI_OBJ_FILES)
	cl $(MSVC_COMP_OPTS_DEBUG) $(SRC_FILES) $(IMGUI_OBJ_FILES) $(MSVC_LINK_OPTS)

msvc-release: $(IMGUI_OBJ_FILES)
	cl $(MSVC_COMP_OPTS_RELEASE) $(SRC_FILES) $(IMGUI_OBJ_FILES) $(MSVC_LINK_OPTS)


CLANG_INC_FLAGS := $(patsubst %,-I %,$(INC_DIRS))

CLANG_COMP_OPTS := -Wall -Wextra -Werror
CLANG_COMP_OPTS += -Wno-nonportable-system-include-path -Wno-reserved-identifier -Wno-cast-function-type
CLANG_COMP_OPTS += -Wno-unsafe-buffer-usage -Wno-language-extension-token -Wno-sign-conversion -Wno-extra-semi-stmt -Wno-cast-align
CLANG_COMP_OPTS += -Wno-cast-qual -Wno-implicit-fallthrough -Wno-string-conversion -Wno-conditional-uninitialized
CLANG_COMP_OPTS += -Wno-declaration-after-statement -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-pedantic
CLANG_COMP_OPTS += -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-zero-as-null-pointer-constant
CLANG_COMP_OPTS += -fsanitize=undefined -D JDP_COMPILER_CLANG
CLANG_COMP_OPTS += $(CLANG_INC_FLAGS) -o $(BUILD_DIR)/$(EXE_FILE)

CLANG_COMP_OPTS_DEBUG := $(CLANG_COMP_OPTS) $(COMP_OPTS_DEBUG) /DJDP_DEBUG_CONSOLE
CLANG_LINK_OPTS_DEBUG := $(LINK_OPTS) /subsystem:console

# CLANG_COMP_OPTS_RELEASE := $(CLANG_COMP_OPTS) $(COMP_OPTS_RELEASE) /DJDP_DEBUG_CONSOLE
# CLANG_LINK_OPTS_RELEASE = $(LINK_OPTS) /subsystem:console

CLANG_COMP_OPTS_RELEASE := $(CLANG_COMP_OPTS) $(COMP_OPTS_RELEASE) -fsanitize-trap=undefined
CLANG_LINK_OPTS_RELEASE = $(LINK_OPTS) /subsystem:windows

clang-debug: $(IMGUI_OBJ_FILES)
	clang-cl $(CLANG_COMP_OPTS_DEBUG) $(SRC_FILES) $(IMGUI_OBJ_FILES) $(CLANG_LINK_OPTS_DEBUG)

clang-release: $(IMGUI_OBJ_FILES)
	clang-cl $(CLANG_COMP_OPTS_RELEASE) $(SRC_FILES) $(IMGUI_OBJ_FILES) $(CLANG_LINK_OPTS_RELEASE)
