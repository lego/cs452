# TODO: It would be nice to pull down all of the makefiles into here, as opposed to having many spread out

# Mark everything as PHONY since compilation is fast, and I don't want to
#   forget to update this every time files are added to sub-projects
.PHONY: arm lib main.elf

all: arm lib main.elf

lib:
	./armcheck; cd lib && make

arm:
	./armcheck; cd arm && make

main.elf:
	./armcheck; cd src && make

clean:
	# clean lib
	cd lib && make clean
	# clean arm
	cd arm && make clean
	# clean main
	cd src && make clean
