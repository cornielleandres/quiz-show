CC = gcc
CFLAGS = -I. -lcurl

quizshow: quizshow.c entities.c
	$(CC) -o quizshow quizshow.c entities.c $(CFLAGS)
