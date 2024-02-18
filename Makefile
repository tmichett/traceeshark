# update wireshark source tree and build
all: copy-source build

# copy only source files to wireshark source tree
copy-source:
	@if [ -d "wireshark/plugins/epan/tracee" ]; then \
        cp plugins/epan/tracee/common.c wireshark/plugins/epan/tracee; \
        cp plugins/epan/tracee/packet-tracee.c wireshark/plugins/epan/tracee; \
        cp plugins/epan/tracee/packet-tracee-network-capture.c wireshark/plugins/epan/tracee; \
        cp plugins/epan/tracee/postdissectors.c wireshark/plugins/epan/tracee; \
        cp plugins/epan/tracee/extractors.c wireshark/plugins/epan/tracee; \
        cp plugins/epan/tracee/tracee.h wireshark/plugins/epan/tracee; \
    else \
        error "Tracee plugin directory doesn't exist, run \"make cmake\" first."; \
    fi

	@if [ -d "wireshark/plugins/wiretap/tracee" ]; then \
        cp plugins/wiretap/tracee/tracee-json.c wireshark/plugins/wiretap/tracee; \
    else \
        error "Tracee plugin directory doesn't exist, run \"make cmake\" first."; \
    fi

# copy all project files to wireshark source tree
copy-all:
	@cp -r plugins wireshark/
	@cp CMakeListsCustom.txt wireshark/

build:
	@if [ -d "wireshark/build" ]; then \
        ninja -C wireshark/build; \
    else \
        echo "Build directory doesn't exist, run \"make cmake\" first."; \
    fi

# update private configuration profile
install:
	@mkdir -p ~/.config/wireshark
	@cp -r profiles ~/.config/wireshark
	@mkdir -p ~/.local/lib/wireshark/extcap
	@cp extcap/tracee-capture.py ~/.local/lib/wireshark/extcap
	@chmod +x ~/.local/lib/wireshark/extcap/tracee-capture.py
	@cp -r extcap/tracee-capture ~/.local/lib/wireshark/extcap
	@mkdir -p ~/.local/lib/wireshark/plugins/epan
	@cp wireshark/build/run/tracee-event.so* ~/.local/lib/wireshark/plugins/epan
	@cp wireshark/build/run/tracee-network-capture.so* ~/.local/lib/wireshark/plugins/epan
	@mkdir -p ~/.local/lib/wireshark/plugins/wiretap
	@cp wireshark/build/run/tracee-json.so* ~/.local/lib/wireshark/plugins/wiretap

# build and run
run: all install
	@wireshark/build/run/wireshark

# build and run with debug logging
debug: all install
	@wireshark/build/run/wireshark --log-level DEBUG

# prepare build directory (needed before building for the first time)
cmake: copy-all
	@rm -rf wireshark/build && mkdir wireshark/build
ifneq ($(USE_QT5),y)
	@cmake -G Ninja -DENABLE_CCACHE=Yes -S wireshark -B wireshark/build
else
	@cmake -G Ninja -DENABLE_CCACHE=Yes -DUSE_qt6=OFF -S wireshark -B wireshark/build
endif

package: all
	@rm -rf package
	@mkdir package
	@cp pkg-install.sh package/
	@cp -r wireshark/build/run package/
	@cp -r profiles package/
	@if [ $(OS_NAME) = "Linux" ]; then\
		cp -r extcap package/; \
	fi
	@cd package && zip -r ../traceeshark-$(shell git rev-parse --short HEAD)-$(shell echo "${OS_NAME}" | tr '[A-Z]' '[a-z]')-$(shell uname -m).zip .

dist: all
	@rm -rf dist/workdir
	@mkdir dist/workdir
	@cp dist/install.sh dist/workdir
	@cp wireshark/build/run/tracee-event.so* dist/workdir
	@cp wireshark/build/run/tracee-network-capture.so* dist/workdir
	@cp wireshark/build/run/tracee-json.so* dist/workdir
	@cp -r profiles dist/workdir
	@if [ $(OS_NAME) = "Linux" ]; then\
		cp -r extcap dist/workdir; \
	fi
	$(eval WS_VERSION := $(shell wireshark/build/run/wireshark --version | grep -o -P "Wireshark \d+\.\d+\.\d+" | grep -o -P "\d+\.\d+\.\d+"))
	@echo $(WS_VERSION) > dist/workdir/ws_version.txt
	@cd dist/workdir && zip -r ../traceeshark-$(shell git describe --tags --abbrev=0)-wireshark-$(WS_VERSION)-$(shell echo "${OS_NAME}" | tr '[A-Z]' '[a-z]')-$(shell uname -m).zip .