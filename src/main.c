#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"

static void print_help(const char* prog_name)
{
	printf("Usage: %s --city <name1,name2,...> [--units <metric|imperial>]\n",
	 prog_name);
}

static void print_weather(const char* city, cJSON* json, const char* units)
{
	if (!json) {
		printf("%s: Error fetching data\n", city);
		return;
	}
	cJSON* current = cJSON_GetObjectItem(json, "current");
	double temp = cJSON_GetObjectItem(current, "temperature_2m")->valuedouble;
	int precip =
	 cJSON_GetObjectItem(current, "precipitation_probability")->valueint;
	printf("%s - %.1f %s - %d%% precipitation\n", city, temp,
	 strcmp(units, "celsius") == 0 ? "C" : "F", precip);
}

int main(int argc, char* argv[])
{
	char* cities_str = NULL;
	const char* units = "celsius";
	int opt;
	struct option long_options[] = {{"city", required_argument, 0, 'c'},
	    {"units", required_argument, 0, 'u'}, {"help", no_argument, 0, 'h'},
	    {0, 0, 0, 0}};

	while ((opt = getopt_long(argc, argv, "c:u:h", long_options, NULL)) != -1) {
		switch (opt) {
		case 'c':
			cities_str = optarg;
			break;
		case 'u':
			units =
			 (strcmp(optarg, "imperial") == 0) ? "fahrenheit" : "celsius";
			break;
		case 'h':
			print_help(argv[0]);
			return EXIT_SUCCESS;
		default:
			print_help(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (!cities_str) {
		fprintf(stderr, "Error: City is required.\n");
		print_help(argv[0]);
		return EXIT_FAILURE;
	}

	// Parse cities into an array
	int city_count = 0;
	size_t cities_str_len = strlen(cities_str);
	char* cities_str_copy = malloc(cities_str_len + 1);
	if (!cities_str_copy) {
		return EXIT_FAILURE;
	}
	memcpy(cities_str_copy, cities_str, cities_str_len + 1);
	char* token = strtok(cities_str_copy, ",");
	while (token) {
		city_count++;
		token = strtok(NULL, ",");
	}
	free(cities_str_copy);

	const char** cities = malloc((size_t) city_count * sizeof(char*));
	token = strtok(cities_str, ",");
	for (int i = 0; i < city_count; i++) {
		cities[i] = token;
		token = strtok(NULL, ",");
	}

	cJSON** results = fetch_weather_multi(cities, city_count, units);
	for (int i = 0; i < city_count; i++) {
		print_weather(cities[i], results[i], units);
		if (results[i]) {
			cJSON_Delete(results[i]);
		}
	}

	free(results);
	free(cities);

	return EXIT_SUCCESS;
}
