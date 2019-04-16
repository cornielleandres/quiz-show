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

struct CurrentGame {
	int level;
	char** random_choices[4 * sizeof(char*)]; // used to randomly shuffle array of answer pointers
	char* correct_choice;
	int end_game;
};

void swap (char** a, char** b);
void randomize(char** arr[], int arr_size);
void print_center_text(WINDOW* window, int row, char *text);
void print_current_level(WINDOW* window, struct CurrentGame current_game, int row, char* money_levels[], int money_levels_len);
static size_t write_trivia_callback(void *contents, size_t size, size_t nmemb, void *userp);
void init_curses();

int main(int argc, char *argv[])
{
	CURL *curl_handle;
	CURLcode res;
	struct TriviaItem trivia;
	struct CurrentGame current_game;
	char guess;
	char valid_guesses[] = "abcd";
	int max_row, max_col, y, x, i, choice;
	char* money_levels[] = {
		"$1,000,000",
		"$500,000",
		"$250,000",
		"$100,000",
		"$50,000",
		"$25,000",
		"$10,000",
		"$5,000",
		"$1,000",
		"$500",
		"$0"
	};
	int money_levels_len = 11;

	current_game.level = 0;
	current_game.end_game = 0;

	// trivia info variables
	char category_str[] = "\"category\":\"";
	char type_str[] = "\",\"type\"";
	char question_str[] = "\"question\":\"";
	char correct_answer_str[] = "\",\"correct_answer\":\"";
	char incorrect_answers_str[] = "\",\"incorrect_answers\":[\"";
	char answer_separator[] = "\",\"";
	char end_str[] = "\"]";
	int	category_str_len,
		category_len,
		question_str_len,
		question_len,
		correct_answer_str_len,
		correct_answer_len,
		incorrect_answers_str_len,
		incorrect_answers_len,
		incorrect_answer1_len,
		answer_separator_len,
		incorrect_answer2_len,
		incorrect_answer3_len;
	char *category_start,
		*type_start,
		*question_start,
		*correct_answer_start,
		*incorrect_answers_start,
		*end_start,
		*incorrect_answers,
		*answer_separator_start,
		*inc_answer2_start,
		*answer_separator2_start,
		*inc_answer3_start;

	init_curses(); // initialize curses
	getmaxyx(stdscr, max_row, max_col); // find the boundaries of the screen

	char url[] = "https://opentdb.com/api.php?amount=1&difficulty=easy&type=multiple";
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

			getyx(main_window, y, x);
			wattrset(main_window, COLOR_PAIR(0));
			print_current_level(main_window, current_game, y, money_levels, money_levels_len);
			getyx(main_window, y, x);
			print_center_text(main_window, y + 2, "Press any key to continue.");
			wrefresh(main_window);
			getch();
			werase(main_window);

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

			category_str_len = strlen(category_str);
			category_start = strstr(trivia.raw_info, category_str);

			/* THIS	CODE IS FOR TESTING PURPOSES ONLY */
			/* COMMENT OUT category_start = ... ABOVE AND UNCOMMENT OUT THE CODE BELOW */
			// char memory[] = "{\"response_code\":0,\"results\":[{\"category\":\"Entertainment: Film\",\"type\":\"multiple\",\"difficulty\":\"easy\",\"question\":\"This movie contains the quote, &quot;Nobody puts Baby in a corner.&quot;\",\"correct_answer\":\"Dirty Dancing\",\"incorrect_answers\":[\"Three Men and a Baby\",\"Ferris Bueller&#039;s Day Off\",\"Pretty in Pink\"]}]}";
			// char decoded_memory[1024];
			// decode_html_entities_utf8(decoded_memory, memory);
			// category_start = strstr(decoded_memory, category_str);

			category_start += category_str_len;
			char type_str[] = "\",\"type\"";
			type_start = strstr(category_start, type_str);
			category_len = type_start - category_start;

			trivia.category = malloc((category_len + 1) * sizeof(char));
			trivia.category[category_len] = '\0';

			i = 0;
			while (i < category_len) trivia.category[i++] = category_start[i];

			question_str_len = strlen(question_str);
			question_start = strstr(type_start, question_str);
			question_start += question_str_len;
			correct_answer_start = strstr(question_start, correct_answer_str);
			question_len = correct_answer_start - question_start;

			trivia.question = malloc((question_len + 1) * sizeof(char));
			trivia.question[question_len] = '\0';

			i = 0;
			while (i < question_len) trivia.question[i++] = question_start[i];

			correct_answer_str_len = strlen(correct_answer_str);
			correct_answer_start += correct_answer_str_len;
			incorrect_answers_start = strstr(correct_answer_start, incorrect_answers_str);
			correct_answer_len = incorrect_answers_start - correct_answer_start;

			trivia.correct_answer = malloc((correct_answer_len + 1) * sizeof(char));
			trivia.correct_answer[correct_answer_len] = '\0';

			current_game.correct_choice = malloc((correct_answer_len + 1) * sizeof(char));
			current_game.correct_choice[correct_answer_len] = '\0';

			i = 0;
			while (i < correct_answer_len)
			{
				trivia.correct_answer[i] = correct_answer_start[i];
				current_game.correct_choice[i++] = correct_answer_start[i];
			}

			incorrect_answers_str_len = strlen(incorrect_answers_str);
			incorrect_answers_start += incorrect_answers_str_len;
			end_start = strstr(incorrect_answers_start, end_str);
			incorrect_answers_len = end_start - incorrect_answers_start;
			incorrect_answers = malloc((incorrect_answers_len + 1) * sizeof(char));
			incorrect_answers[incorrect_answers_len] = '\0';

			i = 0;
			while (i < incorrect_answers_len) incorrect_answers[i++] = incorrect_answers_start[i];

			answer_separator_start = strstr(incorrect_answers, answer_separator);
			incorrect_answer1_len = answer_separator_start - incorrect_answers;

			answer_separator_len = strlen(answer_separator);
			inc_answer2_start = answer_separator_start + answer_separator_len;
			answer_separator2_start = strstr(inc_answer2_start, answer_separator);
			incorrect_answer2_len = answer_separator2_start - inc_answer2_start;

			inc_answer3_start = answer_separator2_start + answer_separator_len;
			incorrect_answer3_len = strlen(inc_answer3_start);

			trivia.incorrect_answer1 = malloc((incorrect_answer1_len + 1) * sizeof(char));
			trivia.incorrect_answer1[incorrect_answer1_len] = '\0';

			i = 0;
			while (i < incorrect_answer1_len) trivia.incorrect_answer1[i++] = incorrect_answers_start[i];

			trivia.incorrect_answer2 = malloc((incorrect_answer2_len + 1) * sizeof(char));
			trivia.incorrect_answer2[incorrect_answer2_len] = '\0';

			i = 0;
			while (i < incorrect_answer2_len) trivia.incorrect_answer2[i++] = inc_answer2_start[i];

			trivia.incorrect_answer3 = malloc((incorrect_answer3_len + 1) * sizeof(char));
			trivia.incorrect_answer3[incorrect_answer3_len] = '\0';

			i = 0;
			while (i < incorrect_answer3_len) trivia.incorrect_answer3[i++] = inc_answer3_start[i];

			current_game.random_choices[0] = &trivia.correct_answer;
			current_game.random_choices[1] = &trivia.incorrect_answer1;
			current_game.random_choices[2] = &trivia.incorrect_answer2;
			current_game.random_choices[3] = &trivia.incorrect_answer3;

			randomize(current_game.random_choices, 4);

			getyx(main_window, y, x);
			wattrset(main_window, COLOR_PAIR(3) | A_BOLD);
			print_center_text(main_window, y + 2, trivia.category);

			wattrset(main_window, COLOR_PAIR(1) | A_BOLD);
			mvwaddstr(main_window, y + 4, 0, trivia.question);

			getyx(main_window, y, x);
			wattrset(main_window, COLOR_PAIR(2) | A_BOLD);
			mvwaddstr(main_window, y + 2, 2, "a) ");
			mvwaddstr(main_window, y + 2, 5, (*current_game.random_choices)[0]);
			mvwaddstr(main_window, y + 4, 2, "b) ");
			mvwaddstr(main_window, y + 4, 5, (*current_game.random_choices)[1]);
			mvwaddstr(main_window, y + 6, 2, "c) ");
			mvwaddstr(main_window, y + 6, 5, (*current_game.random_choices)[2]);
			mvwaddstr(main_window, y + 8, 2, "d) ");
			mvwaddstr(main_window, y + 8, 5, (*current_game.random_choices)[3]);

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
				wclrtoeol(main_window);
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

						if (strcmp((*current_game.random_choices)[choice], current_game.correct_choice) == 0)
						{
							if (++current_game.level == money_levels_len - 1) current_game.end_game = 1;
							wprintw(main_window, "%s is correct!", (*current_game.random_choices)[choice]);
						}
						else
						{
							wprintw(main_window, "Sorry, %s is wrong. It was %s.", (*current_game.random_choices)[choice], current_game.correct_choice);
							current_game.end_game = 1;
						}
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
			free(current_game.correct_choice);
			free(incorrect_answers);
			wattrset(main_window, COLOR_PAIR(1) | A_BOLD);
			getyx(main_window, y, x);
			print_center_text(main_window, y + 2, "Press any key to continue.");
			wrefresh(main_window);
			getch();
			werase(main_window);

			if (current_game.end_game)
			{
				char buffer[21];
				int current_money_level = money_levels_len - current_game.level - 1;
				y = max_row / 2; // get middle row
				print_center_text(main_window, y - 7, "Game over.");
				if (current_money_level == 0)
				{
					snprintf(buffer, strlen(money_levels[current_money_level]) + 36, "YOU'VE WON THE %s!!! Congratulations!", money_levels[current_money_level]);
				}
				else
				{
					snprintf(buffer, strlen(money_levels[current_money_level]) + 10, "You won %s.", money_levels[current_money_level]);
				}
				print_center_text(main_window, y - 5, buffer);
				print_center_text(main_window, y - 3, "Thanks for playing!");
				print_center_text(main_window, y - 1, "Press any key to exit.");
				wrefresh(main_window);
				getch();
				break;
			}
		}
	}
	else
	{
		endwin(); // End curses mode
		fprintf(stderr, "Curl_easy_init() failed.\n");
		return(1);
	}
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

void print_center_text(WINDOW* window, int row, char *text)
{
	int len, indent, y, width;
	getmaxyx(window, y, width); // get screen width
	len = strlen(text); // get text length
	indent = (width - len) / 2; // calculate indent
	mvwaddstr(window, row, indent, text); // print the string
};

void print_current_level(WINDOW* window, struct CurrentGame current_game, int row, char* money_levels[], int money_levels_len)
{
	char buffer[20];
	for (int i = 0; i < money_levels_len; i++)
	{
		if (i == money_levels_len - current_game.level - 1)
		{
			wattrset(window, COLOR_PAIR(3) | A_BOLD);
			snprintf(buffer, strlen(money_levels[i]) + 11, "**** %s ****", money_levels[i]);
		}
		else
		{
			wattrset(window, COLOR_PAIR(0));
			snprintf(buffer, strlen(money_levels[i]) + 1, "%s", money_levels[i]);
		}
		print_center_text(window, row + (2 * i) + 1, buffer);
	}
};

void swap (char** a, char** b) // swap two char pointers
{
	char* temp = *a;
	*a = *b;
	*b = temp;
};

void randomize(char** arr[], int arr_size) // generate a random permutation of arr[]
{
	srand(time(NULL)); // use a diff seed value to not get same result each time this is run

	// start from the last element and swap one by one
	// we don't need to run for the first element, that's why i > 0
	for (int i = arr_size - 1; i > 0; i--)
	{
		int j = rand() % (i + 1); // pick a random index from 0 to i
		swap(&(*arr)[i], &(*arr)[j]); // swap arr[i] with the element at the randomly picked index
	}
};
