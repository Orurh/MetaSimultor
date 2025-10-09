# ====== ultra-minimal wrapper ======
BUILD  ?= build
CONFIG ?= Release
ARCH   ?=                # можно оставить пустым; или, например: ARCH=native

# Формируем общий список аргументов к cmake (без навязывания компиляторов)
CMAKE_ARGS = -DCMAKE_BUILD_TYPE=$(CONFIG)
ifneq ($(strip $(ARCH)),)
CMAKE_ARGS += -DCMAKE_CUDA_ARCHITECTURES=$(ARCH)
endif

.PHONY: all configure build run clean rebuild

all: build

configure:
	cmake -S . -B $(BUILD) $(CMAKE_ARGS)

build: configure
	cmake --build $(BUILD) -j

run: build
	$(BUILD)/disaster_sim

clean:
	rm -rf $(BUILD)

rebuild: clean build
