include config.mk

SRC_DIR := src
OBJ_DIR := build/objs
BIN_DIR := build
TOOLS_SRCDIR := tools
TOOLS_BINDIR := build/tools
TOOLS_CC := $(COMPILER)
TOOLS_CXX := $(COMPILER_CXX)
BUILD_NAME := btcb
EXECUTABLE := $(BIN_DIR)/libmain$(EXE)

LIBS_DIR := lib
LIBS_SRC := $(shell find $(LIBS_DIR)/* -maxdepth 0 -type d -name "*")
LIBS_BIN := $(patsubst $(LIBS_DIR)/%,$(LIBS_DIR)/lib%.a,$(LIBS_SRC))
LIBS_FLAGS := $(patsubst $(LIBS_DIR)/%,-l%,$(LIBS_SRC))
LIBS_BUILD := $(patsubst $(LIBS_DIR)/%,$(LIBS_DIR)/%/build,$(LIBS_SRC))

SRCS := $(shell find $(SRC_DIR) -type f -name "*.c")
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TOOLS_SRC := $(shell find $(TOOLS_SRCDIR) -type f -name "*.c")
TOOLS_EXEC := $(patsubst $(TOOLS_SRCDIR)/%.c,$(TOOLS_BINDIR)/%,$(TOOLS_SRC))
CFLAGS = -Wall -g -I src -I include -I platform/android/include --std=gnu23
LDFLAGS := -L./$(LIBS_DIR) -L ./platform/android/android/lib/arm64-v8a/ -lm -lSDL2 -shared
LIBS :=

BUILD_FILES := $(BIN_DIR) $(LIBS_BIN) $(LIBS_BUILD) src/assets/asset_data.h

CFLAGS += -DLINUX
LIBS += -lm $(LIBS_FLAGS)

CFLAGS += -DNO_VSCODE -DRENDERER_$(RENDERER) -DSDL_VERSION_$(SDL_VERSION)

# Android APK variables
APK_UNALIGNED := build/btcb.unaligned.apk
APK_ALIGNED := build/btcb.aligned.apk
APK_SIGNED := build/btcb.apk
ZIP_UNCOMPRESSED := build/btcb.unsigned.zip

CERT_PEM := platform/android/certificate.pem
KEY_PK8 := platform/android/key.pk8

.PHONY: all clean compile-libs compile-tools run-tools tools compile apk sign-apk

all:
	@$(MAKE) clean --silent
	@$(MAKE) compile-tools --silent
	@$(MAKE) run-tools --silent
	@$(MAKE) compile-libs --silent
	@$(MAKE) compile --silent
	@$(MAKE) apk --silent
	@$(MAKE) sign-apk --silent

tools:
	@$(MAKE) compile-tools --silent
	@$(MAKE) run-tools --silent

compile-libs: $(LIBS_BIN)

$(LIBS_DIR)/lib%.a: $(LIBS_DIR)/%
	@printf "\033[1m\033[32mCompiling library \033[36m$<\033[0m\n"
	@$(MAKE) -f ../../library.mk -C $<
	@cp $</build/out.a $@

compile-tools: $(TOOLS_EXEC)

$(TOOLS_BINDIR)/%: $(TOOLS_SRCDIR)/%.c
	@printf "\033[1m\033[32mCompiling tool \033[36m$< \033[32m-> \033[34m$@\033[0m\n"
	@mkdir -p $(dir $@)
	@$(TOOLS_CC) $(CFLAGS) $< -o $@

$(TOOLS_BINDIR)/%: $(TOOLS_SRCDIR)/%.cpp
	@printf "\033[1m\033[32mCompiling tool \033[36m$< \033[32m-> \033[34m$@\033[0m\n"
	@mkdir -p $(dir $@)
	@$(TOOLS_CXX) $(CFLAGS) $< -o $@

run-tools:
	@for tool in $(TOOLS_EXEC); do \
		printf "\033[1m\033[32mRunning tool \033[36m$$tool\033[0m\n"; \
		$$tool; \
	done

compile: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	@printf "\033[1m\033[32mLinking \033[36m$(OBJ_DIR) \033[32m-> \033[34m$(EXECUTABLE)\033[0m\n"
	@mkdir -p $(BIN_DIR)
	@$(CXX) -shared $(LDFLAGS) -fPIC -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "\033[1m\033[32mCompiling \033[36m$< \033[32m-> \033[34m$@\033[0m\n"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@printf "\033[1m\033[32mCompiling \033[36m$< \033[32m-> \033[34m$@\033[0m\n"
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) -fPIC -c $< -o $@

clean:
	@for i in $(BUILD_FILES); do \
		printf "\033[1m\033[32mDeleting \033[36m$$i \033[32m-> \033[31mX\033[0m\n"; \
		rm -rf $$i; \
	done
	@rm -f $(APK_SIGNED) $(APK_ALIGNED) $(APK_UNALIGNED) $(ZIP_UNCOMPRESSED) $(EXECUTABLE)

-include $(OBJS:.o=.d)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< -o $@ 2> /dev/null

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< -o $@ 2> /dev/null

# APK packaging
ANDROID_MANIFEST := platform/android/android/AndroidManifest.xml
ANDROID_CLASSES := platform/android/android/classes.dex

ANDROID_SDL2 := platform/android/android/lib/arm64-v8a/libSDL2.so

apk: $(EXECUTABLE) $(ANDROID_MANIFEST)
	@printf "\033[1m\033[32mPacking APK with AndroidManifest.xml...\033[0m\n"
	@mkdir -p build/apk/lib/arm64-v8a
	@if [ ! -f $(EXECUTABLE) ]; then echo "Error: libmain.so not found."; exit 1; fi
	@if [ ! -f $(ANDROID_MANIFEST) ]; then echo "Error: AndroidManifest.xml not found."; exit 1; fi
	@cp $(EXECUTABLE) build/apk/lib/arm64-v8a/libmain.so
	@cp $(ANDROID_MANIFEST) build/apk/AndroidManifest.xml
	@cp $(ANDROID_CLASSES) build/apk/classes.dex
	@cp $(ANDROID_SDL2) build/apk/lib/arm64-v8a/libSDL2.so
	@cp $(PREFIX)/lib/libc++_shared.so build/apk/lib/arm64-v8a/libc++_shared.so
	@if ! command -v zip >/dev/null; then echo "Error: zip not installed."; exit 1; fi
	@cd build/apk && zip -r ../btcb.unsigned.zip . || (echo "Error: zip failed."; exit 1)
	@if ! command -v zipalign >/dev/null; then echo "Error: zipalign not installed."; exit 1; fi
	@zipalign -f 4 build/btcb.unsigned.zip $(APK_ALIGNED)

sign-apk: $(APK_ALIGNED) $(CERT_PEM) $(KEY_PK8)
	@printf "\033[1m\033[32mSigning APK...\033[0m\n"
	@if ! command -v apksigner >/dev/null; then echo "Error: apksigner not installed."; exit 1; fi
	@cp $(APK_ALIGNED) $(APK_SIGNED)
	@apksigner sign --cert $(CERT_PEM) --key $(KEY_PK8) $(APK_SIGNED)
	@printf "\033[1m\033[32mSigned APK created: $(APK_SIGNED)\033[0m\n"
