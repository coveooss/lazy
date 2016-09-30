VPATH = ../../lib/coveo/lazy:../../lib/coveo/lazy/detail:../../tests:../../tests/coveo:../../tests/coveo/lazy

all_tests: all_tests.o map_tests.o set_tests.o tests_main.o
	$(CXX) -o all_tests all_tests.o map_tests.o set_tests.o tests_main.o

all_tests.o: all_tests.cpp all_tests.h
	$(CXX) -c -std=c++1z ../../tests/coveo/lazy/all_tests.cpp -I../../lib -I../../tests
map_tests.o: map_tests.cpp map_tests.h
	$(CXX) -c -std=c++1z ../../tests/coveo/lazy/map_tests.cpp -I../../lib -I../../tests
set_tests.o: set_tests.cpp set_tests.h
	$(CXX) -c -std=c++1z ../../tests/coveo/lazy/set_tests.cpp -I../../lib -I../../tests
tests_main.o: tests_main.cpp
	$(CXX) -c -std=c++1z ../../tests/tests_main.cpp -I../../lib -I../../tests

clean:
	rm all_tests all_tests.o map_tests.o set_tests.o tests_main.o

