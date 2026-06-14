#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "include/minicli.h"

typedef struct {
	char* cities_str;
	const char* units;
} WeatherConfig;

static int city_cb(int argc, char** argv, void* user_data)
{
	WeatherConfig* config = (WeatherConfig*) user_data;
	if (argc < 1) {
		fprintf(stderr, "Error: City list missing.\n");
		return 0;
	}
	config->cities_str = argv[0];
	return 1;
}

static int units_cb(int argc, char** argv, void* user_data)
{
	WeatherConfig* config = (WeatherConfig*) user_data;
	if (argc < 1) {
		fprintf(stderr, "Error: Units missing.\n");
		return 0;
	}
	config->units =
	 (strcmp(argv[0], "imperial") == 0) ? "fahrenheit" : "celsius";
	return 1;
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
	WeatherConfig config = {NULL, "celsius"};
	CliParser parser;
	CliInitParams params = {"weather", "Weather forecast tool"};
	cli_init(&parser, params);

	cli_add_argument(&parser,
	 (CliArgument) {"--city", "-c", "Cities to query (comma-separated)",
	     city_cb, &config});
	cli_add_argument(&parser,
	 (CliArgument) {
	     "--units", "-u", "Units (metric|imperial)", units_cb, &config});

	cli_parse(&parser, argc, argv);

	if (!config.cities_str) {
		fprintf(stderr, "Error: City is required.\n");
		cli_print_help(&parser);
		cli_destroy(&parser);
		return EXIT_FAILURE;
	}

	char* cities_str = config.cities_str;
	const char* units = config.units;

	// Parse cities into an array
	int city_count = 0;
	size_t cities_str_len = strlen(cities_str);
	char* cities_str_copy = malloc(cities_str_len + 1);
	if (!cities_str_copy) {
		cli_destroy(&parser);
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
	cli_destroy(&parser);

	return EXIT_SUCCESS;
}
