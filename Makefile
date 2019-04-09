CC = gcc
CFLAGS = -I. -lcurl

quizshow: quizshow.c
	$(CC) -o quizshow quizshow.c $(CFLAGS)
