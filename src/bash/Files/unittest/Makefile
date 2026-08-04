#disable some warning because UT need test functions not in header file.
CFLAGS = -Wno-parentheses -Wno-format-security -Wno-implicit-function-declaration -c
IFLAGS = -I.. -I../include -I../lib
MFLAG = -DDEBUG -DBASH_PLUGIN_UT

all:
	gcc plugin_test.c $(IFLAGS) $(CFLAGS) -o plugin_test.o
	gcc mock_helper.c $(IFLAGS) $(CFLAGS) -o mock_helper.o
	gcc ../plugin.c   $(IFLAGS) $(CFLAGS) $(MFLAG) -o plugin.o
	gcc  plugin_test.o  mock_helper.o plugin.o -o plugin_test  -lc -lcunit

test:
	# run unit test, if UT failed, build will break
	./plugin_test

clean:
	rm *.o
	rm plugin_test
