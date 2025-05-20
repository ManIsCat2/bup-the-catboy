### BUP THE CATBOY CONFIG

RENDERER ?= SDL_RENDERER
MACOS_CROSS ?= 0
MACOS_ARCH ?= $(shell uname -m)
COMPILER ?= clang
COMPILER_CXX ?= clang++
AR ?= ar
SDL_VERSION ?= 3
LIBRARIES ?= sdl$(SDL_VERSION) glew
COMPRESS ?= 0
ANDROID_BUILD := 1

ifeq ($(ANDROID_BUILD),1)
	EXE := .so
	ZIP_UNCOMPRESSED := .uncompressed.zip
	APK_ALIGNED := build/btcb.aligned.apk
	APK_SIGNED := build/btcb.apk
	KEYSTORE := keystore.jks
	KEY_ALIAS := btcb
	KEY_PASS := password
	KEYSTORE_PASS := password
else
	EXE :=
endif

ifeq ($(MACOS_CROSS),1)
	MACOS_TOOL := $(MACOS_ARCH)-apple-$(OSXCROSS_TARGET)
	CC := $(MACOS_TOOL)-$(COMPILER)
	CXX := $(MACOS_TOOL)-$(COMPILER_CXX)
	AR := $(MACOS_TOOL)-$(AR)
else
	CC := $(COMPILER)
	CXX := $(COMPILER_CXX)
endif
