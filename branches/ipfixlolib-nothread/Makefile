# a small makefile to make the library and the example files
# usersend and userreceive.
all: lib test

lib:
	make -C src
test:
	make -C tests
check:
	make -C tests testrun
clean: 
	make -C src/ clean
	make -C tests/ clean

mrproper: clean
	find -iname "*~" | xargs rm -f
