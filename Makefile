
# 'ws' :: cmd-line utility for replacing/swapping 1 type of whitespace-sequence for another. Super fun!
ws: ws.c
	make -C regex lib
	gcc -g ws.c regex/regex.o -o ws.exe
	
ws-clean:
	rm -f ws.exe

clean-all:
	rm -f *.o
	rm -f *.a
	rm -f *.lib
	rm -f *.exe

