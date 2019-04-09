#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

struct TriviaQuestionStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct TriviaQuestionStruct *question = (struct TriviaQuestionStruct *)userp;

	char *ptr = realloc(question->memory, question->size + realsize + 1);

	if(ptr == NULL) {
		// out of memory!
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	question->memory = ptr;
	memcpy(&(question->memory[question->size]), contents, realsize);
	question->size += realsize;
	question->memory[question->size] = 0;

	return realsize;
}

int main(int argc, char *argv[])
{
	CURL *curl_handle;
	CURLcode res;

	struct TriviaQuestionStruct question;
	question.memory = malloc(1); // will be grown as needed by realloc
	question.size = 0; // no data at this point

	char url[] = "https://opentdb.com/api.php?amount=3&category=9&difficulty=easy&type=multiple";

	// initialize the curl session
	curl_handle = curl_easy_init();
	
	if (curl_handle) {
		// specify URL to get
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		// send all data to this function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		// pass the 'question' struct to the callback function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&question);
		// get the data
		res = curl_easy_perform(curl_handle);
		// check for errors
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else {
			printf("%lu bytes retrieved\n", (unsigned long)question.size);
			printf("Question was: %s\n", question.memory);
		}

		// cleanup curl stuff
		curl_easy_cleanup(curl_handle);

		// deallocate memory
		free(question.memory);

		// after being done with libcurl, clean it up
		curl_global_cleanup();
	}

	return 0;
}
