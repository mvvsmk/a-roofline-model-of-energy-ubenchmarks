all:
	nasm -f elf64 -DMAD_PER_ELEMENT=$(MAD_PER_ELEMENT) -o sumsq.o sumsq.asm
	g++ -O2 -DTYPE=$(TYPE) -DMAD_PER_ELEMENT=$(MAD_PER_ELEMENT) -o main_$(MAD_PER_ELEMENT) main.cpp sumsq.o -fopenmp -lgomp

clean:
	rm -f sumsq.o main_$(MAD_PER_ELEMENT)
