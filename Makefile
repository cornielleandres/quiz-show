CC = gcc
CFLAGS = -I. -lcurl -lncursesw

quizshow: quizshow.c entities.c
	$(CC) -o quizshow quizshow.c entities.c $(CFLAGS)
