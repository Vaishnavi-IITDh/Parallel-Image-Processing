# Build rules
build-part1: part1.out
build-part2.1:part2.1.out
build-part2.2: part2.2.out
build-part2.3: part2.3.out

# Compilation rules
part1.out: part1.cpp
	g++ -g -o part1.out part1.cpp libppm.cpp

part2.1.out: part2.1.cpp
	g++ -g -o part2.1.out part2.1.cpp libppm.cpp

part2.2.out: part2.2.1.cpp part2.2.2.cpp part2.2.3.cpp 
	g++ cleanup.cpp -o cleanup.out
	g++ -g -o part2.2.1.out part2.2.1.cpp libppm.cpp
	g++ -g -o part2.2.2.out part2.2.2.cpp libppm.cpp
	g++ -g -o part2.2.3.out part2.2.3.cpp libppm.cpp

part2.3.out: part2.3.cpp
	g++ -g -o part2.3.out part2.3.cpp libppm.cpp

# Run rules
part1: part1.out
	./part1.out ../images/1.ppm ../images/output_part1.ppm

part2_1:part2.1.out
	./part2.1.out ../images/1.ppm ../images/output_part2_1.ppm

part2_2: part2.2.out
	./run.sh ../images/1.ppm ../images/output_part2_2.ppm

part2_3:part2.3.out
	./part2.3.out ../images/1.ppm ../images/output_part2_3.ppm

# Cleaning rules
clean-part1:
	rm part1.out

clean-part2.1:
	rm part2.1.out

clean-part2.2:
	rm part2.2.1.out part2.2.2.out part2.2.3.out cleanup.out

clean-part2.3:
	rm part2.3.out

clean:
	rm *.out
