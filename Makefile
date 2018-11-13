convert: 
	gcc -o convert convert.c -L. -lmp3lame -lpthread -I ./lame-3.100/include/

clean:
	rm convert
