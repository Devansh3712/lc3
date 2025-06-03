sum:
	gcc -Wall src/sum.c -o sum.out
	./sum.out

vm:
	gcc -Wall src/vm.c src/vm_debug.c -o vm.out

clean:
	rm *.out *.obj
