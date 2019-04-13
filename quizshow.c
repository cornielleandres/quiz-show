#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <time.h>

#include <ncursesw/curses.h> // <curses.h> automatically includes <stdio.h> and <unctrl.h>
#include <curl/curl.h>
#include "entities.h"


struct TriviaItem {
	char* raw_info;
	size_t size;
	char* category;
	char* question;
	char* correct_answer;
	char* incorrect_answer1;
	char* incorrect_answer2;
	char* incorrect_answer3;
};

struct CurrentRound {
	char* choices[4];
};

void swap (char** a, char** b);
void randomize(char* arr[], int arr_size);
void print_center_text(WINDOW* window, int row, char *text);
void set_trivia_info(struct TriviaItem* trivia, struct CurrentRound* round);
static size_t write_trivia_callback(void *contents, size_t size, size_t nmemb, void *userp);
void init_curses();

int main(int argc, char *argv[])
{
	CURL *curl_handle;
	CURLcode res;
	struct TriviaItem trivia;
	struct CurrentRound round;
	char guess;
	char valid_guesses[] = "abcd";
	int max_row, max_col, y, x, i, choice;

	init_curses(); // initialize curses
	getmaxyx(stdscr, max_row, max_col); // find the boundaries of the screen

	char url[] = "https://opentdb.com/api.php?amount=1&category=9&difficulty=easy&type=multiple";
	curl_global_init(CURL_GLOBAL_ALL); // set up the program environment that libcurl needs
	curl_handle = curl_easy_init(); // initialize the curl session

	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_trivia_callback); // send data here
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&trivia); // pass trivia struct
		curl_easy_setopt(curl_handle, CURLOPT_URL, url); // specify URL to get

		WINDOW* main_window = subwin(stdscr, max_row - 4, max_col - 4, 3, 2);
		border(
			ACS_VLINE, // left side
			ACS_VLINE, // right side
			ACS_HLINE, // top
			ACS_HLINE, // bottom
			ACS_ULCORNER, // upper-left corner
			ACS_URCORNER, // upper-right corner
			ACS_LLCORNER, // lower-left corner
			ACS_LRCORNER // lower-right corner
		);
		attrset(COLOR_PAIR(1) | A_BOLD);
		print_center_text(stdscr, 2, "Welcome to the Quiz Show!");
		refresh(); // refresh stdscr to show border and title

		while (TRUE)
		{
			trivia.raw_info = malloc(1); // will be grown as needed by realloc
			trivia.size = 0; // no data at this point
			wattrset(main_window, COLOR_PAIR(2) | A_BOLD);
			print_center_text(main_window, 2, "Fetching trivia question...");
			wrefresh(main_window);
			res = curl_easy_perform(curl_handle); // get the data
			if (res != CURLE_OK) { // check for errors
				endwin(); // End curses mode
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				return(1);
			}
			wmove(main_window, 2, 0);
			wclrtoeol(main_window);
			wattrset(main_window, COLOR_PAIR(2) | A_BOLD);
			waddstr(main_window, trivia.raw_info);
			decode_html_entities_utf8(trivia.raw_info, NULL);
			set_trivia_info(&trivia, &round);
			randomize(round.choices, 4);

			getyx(main_window, y, x);
			wattrset(main_window, COLOR_PAIR(3) | A_BOLD);
			print_center_text(main_window, y + 2, trivia.category);

			wattrset(main_window, COLOR_PAIR(1) | A_BOLD);
			mvwaddstr(main_window, y + 4, 0, trivia.question);

			wattrset(main_window, COLOR_PAIR(2) | A_BOLD);

			getyx(main_window, y, x);
			mvwaddstr(main_window, y + 2, 2, "a) ");
			mvwaddstr(main_window, y + 2, 5, round.choices[0]);

			mvwaddstr(main_window, y + 4, 2, "b) ");
			mvwaddstr(main_window, y + 4, 5, round.choices[1]);

			mvwaddstr(main_window, y + 6, 2, "c) ");
			mvwaddstr(main_window, y + 6, 5, round.choices[2]);

			mvwaddstr(main_window, y + 8, 2, "d) ");
			mvwaddstr(main_window, y + 8, 5, round.choices[3]);

			wattrset(main_window, COLOR_PAIR(1) | A_BOLD);
			print_center_text(main_window, y + 10, "What is your answer (a, b, c, d)?");
			wrefresh(main_window);
			getyx(main_window, y, x);
			wattrset(main_window, COLOR_PAIR(3) | A_BOLD);
			i = 4;
			choice = 4; // choice defaults to 4 which would make it an invalid guess
			while (choice == 4)
			{
				guess = getch(); // get a single character from user
				guess = tolower(guess);
				i = 0;
				wmove(main_window, y + 2, 2);
				while (valid_guesses[i]) // while going through all the valid guesses
				{
					if (guess == valid_guesses[i++]) { // if user made a valid guess
						switch(guess)
						{
							case 'a':
							default:
								choice = 0;
								break;
							case 'b':
								choice = 1;
								break;
							case 'c':
								choice = 2;
								break;
							case 'd':
								choice = 3;
								break;
						}
						if (strcmp(round.choices[choice], trivia.correct_answer) == 0)
						{
							wprintw(main_window, "%s is correct!", round.choices[choice]);
						}
						else
						{
							wprintw(main_window, "Sorry, %s is wrong. The correct answer was %s.", round.choices[choice], trivia.correct_answer);
						}
						wrefresh(main_window);
						break;
					}
				}
				if (choice == 4) // if user made no valid guess
				{
					wprintw(main_window, "%c is an invalid guess. Please try again.", guess);
					wrefresh(main_window);
				}
			}
			free(trivia.raw_info);
			free(trivia.category);
			free(trivia.question);
			free(trivia.correct_answer);
			free(trivia.incorrect_answer1);
			free(trivia.incorrect_answer2);
			free(trivia.incorrect_answer3);
			wattrset(main_window, COLOR_PAIR(1) | A_BOLD);
			getyx(main_window, y, x);
			print_center_text(main_window, y + 2, "Press any key to continue.");
			wrefresh(main_window);
			getch();
			werase(main_window);
		}
	}
	else
	{
		endwin(); // End curses mode
		fprintf(stderr, "Curl_easy_init() failed.\n");
		return(1);
	}
	free(trivia.raw_info);
	free(trivia.category);
	free(trivia.question);
	free(trivia.correct_answer);
	free(trivia.incorrect_answer1);
	free(trivia.incorrect_answer2);
	free(trivia.incorrect_answer3);
	curl_easy_cleanup(curl_handle); // cleanup curl stuff
	curl_global_cleanup(); // after being done with libcurl, clean it up
	endwin(); // End curses mode
	return(0);
};

void init_curses()
{
	setlocale(LC_ALL, "C.UTF-8");

	initscr(); // start curses mode
	cbreak(); // allow for control characters like suspend (CTRL-Z), interrupt and quit (CTRL-C)
	noecho(); // don't echo() while doing a getch
	keypad(stdscr, TRUE); // enable the reading of function keys like F1, F2, arrow keys etc.
	start_color(); // start color functionality

	// set definitions for color-pairs
	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
};

static size_t write_trivia_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct TriviaItem *trivia = (struct TriviaItem *)userp;

	char *ptr = realloc(trivia->raw_info, trivia->size + realsize + 1);

	if(ptr == NULL) {
		// out of memory!
		printw("not enough memory (realloc returned NULL)\n");
		return(0);
	}

	trivia->raw_info = ptr;
	memcpy(&(trivia->raw_info[trivia->size]), contents, realsize);
	trivia->size += realsize;
	trivia->raw_info[trivia->size] = 0;

	return realsize;
};

void set_trivia_info(struct TriviaItem* trivia, struct CurrentRound* round)
{
	char category_str[] = "\"category\":\""; // string to capture category
	int category_str_len = strlen(category_str);
	char* category_start = strstr(trivia->raw_info, category_str);
	category_start += category_str_len;
	char type_str[] = "\",\"type\"";
	char* type_start = strstr(category_start, type_str);
	int category_len = type_start - category_start;

	trivia->category = (char*) malloc((category_len + 1) * sizeof(char));
	trivia->category[category_len] = '\0';

	int i = 0;
	while (i != category_len) trivia->category[i++] = category_start[i];

	char question_str[] = "\"question\":\"";
	int question_str_len = strlen(question_str);
	char correct_answer_str[] = "\",\"correct_answer\":\"";
	char* question_start = strstr(type_start, question_str);
	question_start += question_str_len;
	char* correct_answer_start = strstr(question_start, correct_answer_str);
	int question_len = correct_answer_start - question_start;

	trivia->question = (char*) malloc((question_len + 1) * sizeof(char));
	trivia->question[question_len] = '\0';

	i = 0;
	while (i != question_len) trivia->question[i++] = question_start[i];

	int correct_answer_str_len = strlen(correct_answer_str);
	char incorrect_answers_str[] = "\",\"incorrect_answers\":[\"";
	correct_answer_start += correct_answer_str_len;
	char* incorrect_answers_start = strstr(correct_answer_start, incorrect_answers_str);
	int correct_answer_len = incorrect_answers_start - correct_answer_start;

	trivia->correct_answer = (char*) malloc((correct_answer_len + 1) * sizeof(char));
	trivia->correct_answer[correct_answer_len] = '\0';

	i = 0;
	while (i < correct_answer_len) trivia->correct_answer[i++] = correct_answer_start[i];

	round->choices[0] = trivia->correct_answer;

	int incorrect_answers_str_len = strlen(incorrect_answers_str);
	char end_str[] = "\"]";
	incorrect_answers_start += incorrect_answers_str_len;
	char* end_start = strstr(incorrect_answers_start, end_str);
	int incorrect_answers_len = end_start - incorrect_answers_start;
	char incorrect_answers[incorrect_answers_len];
	strncpy(incorrect_answers, incorrect_answers_start, incorrect_answers_len);
	incorrect_answers[incorrect_answers_len] = '\0';

	char answer_separator[] = "\",\"";
	char* answer_separator_start = strstr(incorrect_answers, answer_separator);
	int incorrect_answer1_len = answer_separator_start - incorrect_answers;

	int answer_separator_len = strlen(answer_separator);
	char* inc_answer2_start = answer_separator_start + answer_separator_len;
	char* answer_separator2_start = strstr(inc_answer2_start, answer_separator);
	int incorrect_answer2_len = answer_separator2_start - inc_answer2_start;

	char* inc_answer3_start = answer_separator2_start + answer_separator_len;
	int incorrect_answer3_len = strlen(inc_answer3_start);

	trivia->incorrect_answer1 = (char*) malloc((incorrect_answer1_len + 1) * sizeof(char));
	trivia->incorrect_answer1[incorrect_answer1_len] = '\0';

	i = 0;
	while (i < incorrect_answer1_len) trivia->incorrect_answer1[i++] = incorrect_answers_start[i];

	round->choices[1] = trivia->incorrect_answer1;

	trivia->incorrect_answer2 = (char*) malloc((incorrect_answer2_len + 1) * sizeof(char));
	trivia->incorrect_answer2[incorrect_answer2_len] = '\0';

	i = 0;
	while (i < incorrect_answer2_len) trivia->incorrect_answer2[i++] = inc_answer2_start[i];

	round->choices[2] = trivia->incorrect_answer2;

	trivia->incorrect_answer3 = (char*) malloc((incorrect_answer3_len + 1) * sizeof(char));
	trivia->incorrect_answer3[incorrect_answer3_len] = '\0';

	i = 0;
	while (i < incorrect_answer3_len) trivia->incorrect_answer3[i++] = inc_answer3_start[i];

	round->choices[3] = trivia->incorrect_answer3;
};

void print_center_text(WINDOW* window, int row, char *text)
{
	int len, indent, y, width;
	getmaxyx(window, y, width); // get screen width
	len = strlen(text); // get text length
	indent = (width - len)/2; // calculate indent
	mvwaddstr(window, row, indent, text); // print the string
};

void swap (char** a, char** b) // swap two char pointers
{ 
	char* temp = *a;
	*a = *b;
	*b = temp;
};

void randomize(char* arr[], int arr_size) // generate a random permutation of arr[]
{
	srand(time(NULL)); // use a diff seed value to not get same result each time this is run

	// start from the last element and swap one by one
	// we don't need to run for the first element, that's why i > 0
	for (int i = arr_size - 1; i > 0; i--)
	{
		int j = rand() % (i + 1); // pick a random index from 0 to i
		swap(&arr[i], &arr[j]); // Swap arr[i] with the element at the randomly picked index
	}
};
