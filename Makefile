# Mark everything as PHONY since compilation is fast, and I don't want to
#   forget to update this every time files are added to sub-projects
.PHONY: lib main.elf

all: lib main.elf

lib:
	./armcheck; cd lib && make

main.elf:
	./armcheck; cd src && make

clean:
	# clean bwio
	cd lib && make clean
	# clean main
	cd src && make clean
