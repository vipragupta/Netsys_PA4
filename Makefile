CC=gcc
MD5_Flag1 = -lssl
MD5_Flag2 = -lcrypto

SRC= webproxy.c
 

all: $(SRC)
	$(CC) $(SRC) $(MD5_Flag1) $(MD5_Flag2) -o proxy.o

.PHONY : clean
clean :
	-rm -f *.o 
