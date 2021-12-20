all: testlib shared_lib_test shared_lib_test_scorep

testlib: testlib.c
	clang++ -finstrument-functions -fPIC -shared testlib.c -o libtestlib.so

shared_lib_test: shared_lib_test.c libtestlib.so
	../../build/scripts/clang-inst++ -v -mllvm -filter-file="basic.filt" -O2 shared_lib_test.c -L../../build/lib/calltree_verifier -lcalltreeverifier_static -L./ -Wl,-rpath,./ -ltestlib -ltbb -o shared_lib_test

shared_lib_test_scorep: shared_lib_test.c
	scorep-config --mpp=mpi --adapter-init > scorep_init.c
	clang -c scorep_init.c -o scorep_init.o
	../../build/scripts/clang-inst++ -c -v -mllvm -filter-file="basic.filt" -O2 -fPIE shared_lib_test.c -o shared_lib_test.o
	clang++ -v scorep_init.o -Wl,-start-group `scorep-config --ldflags` `scorep-config --libs --mpp=mpi --compiler` -Wl,-end-group -L./ -Wl,-rpath,./ -ltestlib -ltbb shared_lib_test.o -o shared_lib_test_scorep
	rm scorep_init.c scorep_init.o

clean:
	rm *.so *.o

