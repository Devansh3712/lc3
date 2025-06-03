sum:
	gcc -Wall sum.c -o sum.out
	./sum.out

vm:
	gcc -Wall main.c -o vm.out

clean:
	rm *.out *.obj
