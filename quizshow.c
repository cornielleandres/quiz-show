#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include "entities.h"

struct TriviaStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct TriviaStruct *trivia = (struct TriviaStruct *)userp;

	char *ptr = realloc(trivia->memory, trivia->size + realsize + 1);

	if(ptr == NULL) {
		// out of memory!
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	trivia->memory = ptr;
	memcpy(&(trivia->memory[trivia->size]), contents, realsize);
	trivia->size += realsize;
	trivia->memory[trivia->size] = 0;

	return realsize;
}

int main(int argc, char *argv[])
{
	CURL *curl_handle;
	CURLcode res;

	struct TriviaStruct trivia;
	trivia.memory = malloc(1); // will be grown as needed by realloc
	trivia.size = 0; // no data at this point

	char url[] = "https://opentdb.com/api.php?amount=1&category=9&difficulty=easy&type=multiple";

	// initialize the curl session
	curl_handle = curl_easy_init();

	if (curl_handle) {
		// specify URL to get
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		// send all data to this function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		// pass the 'trivia' struct to the callback function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&trivia);
		// get the data
		res = curl_easy_perform(curl_handle);
		// check for errors
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else {
			printf("TRIVIA BEFORE DECODE: %s\n\n", trivia.memory);
			decode_html_entities_utf8(trivia.memory, NULL);
		}
		// cleanup curl stuff
		curl_easy_cleanup(curl_handle);

		char category_str[] = "\"category\":\"";
		int category_str_len = strlen(category_str);
		char type_str[] = "\",\"type\"";
		char* category_start = strstr(trivia.memory, category_str);

		category_start += category_str_len;
		char* type_start = strstr(category_start, type_str);
		int category_length = type_start - category_start;
		char category[category_length];
		strncpy(category, category_start, category_length);
		category[category_length] = '\0';
		printf("CATEGORY: %s\n\n", category);

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
		printf("QUESTION: %s\n\n", question);

		int correct_answer_str_len = strlen(correct_answer_str);
		char incorrect_answers_str[] = "\",\"incorrect_answers\":[\"";
		correct_answer_start += correct_answer_str_len;
		char* incorrect_answers_start = strstr(correct_answer_start, incorrect_answers_str);
		int correct_answer_length = incorrect_answers_start - correct_answer_start;
		char correct_answer[correct_answer_length];
		strncpy(correct_answer, correct_answer_start, correct_answer_length);
		correct_answer[correct_answer_length] = '\0';
		printf("CORRECT ANSWER: %s\n\n", correct_answer);

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

		printf("INCORRECT ANSWER #1: %s\n\n", incorrect_answer1);
		printf("INCORRECT ANSWER #2: %s\n\n", incorrect_answer2);
		printf("INCORRECT ANSWER #3: %s\n\n", incorrect_answer3);

		// char name[20];
		// printf("Hello. What's your name?\n");
		// //scanf("%s", &name);  - deprecated
		// fgets(name,20,stdin);
		// printf("Hi there, %s", name);

		// deallocate memory
		free(trivia.memory);
		// after being done with libcurl, clean it up
		curl_global_cleanup();
	}

	return 0;
}
