# Mark everything as PHONY since compilation is fast, and I don't want to
#   forget to update this every time files are added to sub-projects
.PHONY: libbwio.a main.elf

all: libbwio.a main.elf

libbwio.a:
	cd bwio/src && make

main.elf:
	cd src && make

clean:
	# clean bwio
	cd bwio/src && make clean
	# clean main
	cd src && make clean
