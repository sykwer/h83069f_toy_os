.PHONY: all
all:
		make -C src/os
		make -C src/bootload
		make image -C src/bootload

.PHONY: write
		make write -C src/bootload

.PHONY: clean
clean:
		make clean -C src/os
		make clean -C src/bootload
