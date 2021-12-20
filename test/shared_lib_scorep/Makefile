scorep_exe=$(shell which scorep)
scorep_dir=$(shell dirname ${scorep_exe})/..
constructor=$(shell dirname ${scorep_exe})/../lib/scorep/scorep_constructor.o

all: testlib shared_lib_test shared_lib_test_scorep shared_lib_test_scorep2 shared_lib_test_scorep_helper

testlib: testlib.c
	clang++ -finstrument-functions -fPIC -shared testlib.c -o libtestlib.so

shared_lib_test: shared_lib_test.c libtestlib.so
	../../build-release/scripts/clang-inst++ -v -mllvm -filter-file="basic.filt" -O2 shared_lib_test.c -L../../build-release/lib/calltree_verifier -lcalltreeverifier_static -L./ -Wl,-rpath,./ -ltestlib -ltbb -o shared_lib_test

shared_lib_test_scorep: shared_lib_test.c testlib
	@echo "ScoreP: $(scorep_exe)"
	@echo "Using constructor: $(constructor)"
	scorep-config --mpp=none --adapter-init > scorep_init.c
	clang -c scorep_init.c -o scorep_init.o
	../../build-release/scripts/clang-inst++ -c -v -mllvm -filter-file="basic.filt" -O2 -fPIE shared_lib_test.c -o shared_lib_test.o
	clang++ -v -L $(scorep_dir)/lib -Wl,-start-group `scorep-config --libs --mpp=none --compiler` $(constructor) -Wl,-end-group -L./ -Wl,-rpath,./ -ltestlib -ltbb scorep_init.o shared_lib_test.o -o shared_lib_test_scorep
	rm scorep_init.c scorep_init.o

shared_lib_test_scorep_helper: shared_lib_test.c testlib
	@echo "ScoreP: $(scorep_exe)"
	@echo "Using constructor: $(constructor)"
	scorep-config --mpp=none --adapter-init > scorep_init.c
	clang -c scorep_init.c -o scorep_init.o
	../../build-release/scripts/clang-inst++ -c -v -mllvm -filter-file="basic.filt" -O2 -fPIE shared_lib_test.c -o shared_lib_test.o
	clang++ -v -L $(scorep_dir)/lib -Wl,-start-group `scorep-config --libs --mpp=none --compiler` $(constructor) -Wl,-end-group -L../../build-release/lib/scorep_helper -Wl,-rpath,../../build-release/lib/scorep_helper -lscorep_helper -L./ -Wl,-rpath,./ -ltestlib -ltbb scorep_init.o shared_lib_test.o -o shared_lib_test_scorep_helper
	rm scorep_init.c scorep_init.o


clean:
	rm shared_lib_test_scorep
	rm shared_lib_test_scorep2
	rm shared_lib_test
	rm *.so *.o

