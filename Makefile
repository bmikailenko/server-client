CC=gcc
CFLAGS=-I
DEPS=mftp.h

all: mftp mftpserve

mftp: mftp.c
	$(CC) -o mftp mftp.c

mftpserve: mftpserve.c
	$(CC) -o mftpserve mftpserve.c

clean:
	rm -r mftpserve mftp
