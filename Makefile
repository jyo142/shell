fsh: fsh.c
	gcc -o fsh fsh.c -g -Wall -std=gnu99 -L/usr/local/lib -I/usr/local/include -lreadline
