all:
	./build.sh
	AFL_DEMO=1 ./AFLplusplus/afl-clang-fast -O0 -w program.c -o program
	mkdir -p input
	echo -ne "\x00" > input/seed0

run:
	AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 AFLplusplus/afl-fuzz -i input -o output -- ./program

clean:
	rm -rf AFLplusplus input output program
