vita3k := ~/Applications/Vita3K-x86_64.AppImage
app		 := snake

.PHONY: clean build debug

all: clean build

clean:
	rm -rf build

build:
	mkdir -p build
	cd build && cmake .. && make

debug: build
	$(vita3k) --console --log-level 1 build/${app}.vpk -r VSDK00007
