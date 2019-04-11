#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <ncursesw/curses.h>
#include <curl/curl.h>
#include "entities.h"


struct TriviaStruct {
	char* info;
	size_t size;
};

static size_t WriteTriviaCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct TriviaStruct *trivia = (struct TriviaStruct *)userp;

	char *ptr = realloc(trivia->info, trivia->size + realsize + 1);

	if(ptr == NULL) {
		// out of memory!
		printw("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	trivia->info = ptr;
	memcpy(&(trivia->info[trivia->size]), contents, realsize);
	trivia->size += realsize;
	trivia->info[trivia->size] = 0;

	return realsize;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "C.UTF-8");
	initscr(); // start curses mode
	cbreak();
	noecho();
	start_color(); // start color functionality
	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);

	border('a','b','c','d','*','*','*','*');
	move(2, 2);
	attrset(COLOR_PAIR(1) | A_BOLD);
	addstr("Welcome to the Quiz Show!\n");
	move(4, 4);

	CURL *curl_handle;
	CURLcode res;

	struct TriviaStruct trivia;
	trivia.info = malloc(1); // will be grown as needed by realloc
	trivia.size = 0; // no data at this point

	char url[] = "https://opentdb.com/api.php?amount=1&category=9&difficulty=easy&type=multiple";
	char guess;

	curl_handle = curl_easy_init(); // initialize the curl session

	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url); // specify URL to get
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteTriviaCallback); // send data here
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&trivia); // pass trivia struct
		res = curl_easy_perform(curl_handle); // get the data
		if (res != CURLE_OK) { // check for errors
			wprintw(stdscr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
			attrset(COLOR_PAIR(2) | A_BOLD);
			printw("TRIVIA BEFORE DECODE: %s\n\n", trivia.info);
			decode_html_entities_utf8(trivia.info, NULL);
		}
		curl_easy_cleanup(curl_handle); // cleanup curl stuff

		attrset(COLOR_PAIR(3) | A_BOLD);

		/* 
			- First two parameters specify the position at which to start 
			- Third parameter number of characters to update. -1 means till end of line
			- Forth parameter is the normal attribute you wanted to give to the charcter
			- Fifth is the color index. It is the index given during init_pair()
			- Use 0 if you didn't want color
			- Sixth one is always NULL 
		*/
		mvchgat(10, 3, -1, A_BOLD, 1, NULL);

		char category_str[] = "\"category\":\"";
		int category_str_len = strlen(category_str);
		char type_str[] = "\",\"type\"";
		char* category_start = strstr(trivia.info, category_str);

		category_start += category_str_len;
		char* type_start = strstr(category_start, type_str);
		int category_length = type_start - category_start;
		char category[category_length];
		strncpy(category, category_start, category_length);
		category[category_length] = '\0';
		printw("CATEGORY: %s\n\n", category);

		char question_str[] = "\"question\":\"";
		int question_str_len = strlen(question_str);
		char correct_answer_str[] = "\",\"correct_answer\":\"";
		char* question_start = strstr(type_start, question_str);
		question_start += question_str_len;
		char* correct_answer_start = strstr(question_start, correct_answer_str);
		int question_length = correct_answer_start - question_start;
		char question[question_length];
		strncpy(question, question_start, question_length);
		question[question_length] = '\0';
		printw("QUESTION: %s\n\n", question);

		int correct_answer_str_len = strlen(correct_answer_str);
		char incorrect_answers_str[] = "\",\"incorrect_answers\":[\"";
		correct_answer_start += correct_answer_str_len;
		char* incorrect_answers_start = strstr(correct_answer_start, incorrect_answers_str);
		int correct_answer_length = incorrect_answers_start - correct_answer_start;
		char correct_answer[correct_answer_length];
		strncpy(correct_answer, correct_answer_start, correct_answer_length);
		correct_answer[correct_answer_length] = '\0';
		printw("CORRECT ANSWER: %s\n\n", correct_answer);

		int incorrect_answers_str_len = strlen(incorrect_answers_str);
		char end_str[] = "]";
		incorrect_answers_start += incorrect_answers_str_len;
		char* end_start = strstr(incorrect_answers_start, end_str);
		int incorrect_answers_length = end_start - incorrect_answers_start;
		char incorrect_answers[incorrect_answers_length];
		strncpy(incorrect_answers, incorrect_answers_start, incorrect_answers_length);
		incorrect_answers[incorrect_answers_length] = '\0';

		char answer_separator[] = "\",\"";
		char* answer_separator_start = strstr(incorrect_answers, answer_separator);
		int incorrect_answer1_len = answer_separator_start - incorrect_answers;
		char incorrect_answer1[incorrect_answer1_len];
		strncpy(incorrect_answer1, incorrect_answers, incorrect_answer1_len);
		incorrect_answer1[incorrect_answer1_len] = '\0';

		int answer_separator_len = strlen(answer_separator);
		char* incorrect_answer2_start = answer_separator_start + answer_separator_len;
		char* answer_separator2_start = strstr(incorrect_answer2_start, answer_separator);
		int incorrect_answer2_len = answer_separator2_start - incorrect_answer2_start;
		char incorrect_answer2[incorrect_answer2_len];
		strncpy(incorrect_answer2, incorrect_answer2_start, incorrect_answer2_len);
		incorrect_answer2[incorrect_answer2_len] = '\0';

		char* incorrect_answer3_start = answer_separator2_start + answer_separator_len;
		int incorrect_answer3_len = strlen(incorrect_answer3_start);
		char incorrect_answer3[incorrect_answer3_len];
		strncpy(incorrect_answer3, incorrect_answer3_start, incorrect_answer3_len);
		incorrect_answer3[incorrect_answer3_len - 1] = '\0';

		printw("INCORRECT ANSWER #1: %s\n\n", incorrect_answer1);
		printw("INCORRECT ANSWER #2: %s\n\n", incorrect_answer2);
		printw("INCORRECT ANSWER #3: %s\n\n", incorrect_answer3);

		free(trivia.info); // deallocate memory
		curl_global_cleanup(); // after being done with libcurl, clean it up
	}

	attrset(COLOR_PAIR(1) | A_BOLD);
	addstr("WHAT IS YOUR GUESS?\n");
	refresh();
	guess = getch();
	attrset(COLOR_PAIR(2) | A_BOLD);
	printw("YOU GUESSED: %c\n\n", guess);
	attrset(COLOR_PAIR(3) | A_BOLD);
	addstr("PRESS ANY KEY TO EXIT");
	getch();
	endwin(); // End curses mode
	return(0);
}
