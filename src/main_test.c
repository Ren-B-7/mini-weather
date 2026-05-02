#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "convert.h"
#include "currencies.h"

#define CURRENCY_CODE_LEN 8
#define TARGET_CURRENCIES_MAX_LEN 256

/**
 * Prints help and usage information.
 */
static void print_help(const char* prog_name)
{
	printf("Usage: %s [options] <value> <Current currency> <Desired "
	       "currency(ies)>\n",
	 prog_name);
	printf("\nOptions:\n");
	printf("  -h, --help        Show this help message\n");
	printf("  -l, --list        List all supported currency codes and names\n");
	printf("  -s, --search <q>  Search for a currency code or name\n");
	printf("  -t, --test <iso>  Test internal currency list against API for "
	       "base <iso>\n");
	printf("\nPositional Arguments:\n");
	printf("  <Desired currency(ies)> can be a single code (e.g., USD) or a "
	       "comma-separated list (e.g., EUR,ZAR,XCD)\n");
	printf("\nExample:\n");
	printf("  %s 100 USD EUR,ZAR,XCD\n", prog_name);
}

/**
 * Uppercases a string in-place.
 */
static void uppercase_string(char* str)
{
	if (str == NULL) {
		return;
	}
	for (char* ptr = str; *ptr != '\0'; ++ptr) {
		*ptr = (char) toupper((unsigned char) *ptr);
	}
}

/**
 * Entry point for the exchange CLI.
 */
int main(int argc, char* argv[])
{
	char* endptr;
	char current_currency[CURRENCY_CODE_LEN];
	char target_currencies_str[TARGET_CURRENCIES_MAX_LEN];
	cJSON* json = NULL;
	char* current_pos = NULL;
	char* next_comma = NULL;
	double value;

	if (argc < 2) {
		print_help(argv[0]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		print_help(argv[0]);
		return EXIT_SUCCESS;
	}

	if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
		list_currencies();
		return EXIT_SUCCESS;
	}

	if (strcmp(argv[1], "--search") == 0 || strcmp(argv[1], "-s") == 0) {
		if (argc < 3) {
			fprintf(stderr, "Error: Search query missing.\n");
			return EXIT_FAILURE;
		}
		search_currencies(argv[2]);
		return EXIT_SUCCESS;
	}

	/* Handle --test */
	if (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0) {
		char test_base[CURRENCY_CODE_LEN];
		if (argc < 3) {
			fprintf(stderr, "Error: ISO code missing for test.\n");
			return EXIT_FAILURE;
		}
		snprintf(test_base, sizeof(test_base), "%s", argv[2]);
		uppercase_string(test_base);
		if (!is_supported_currency(test_base)) {
			fprintf(stderr, "Error: Unsupported current currency code '%s'.\n",
			 test_base);
			return EXIT_FAILURE;
		}
		test_currencies_against_api(test_base);
		return EXIT_SUCCESS;
	}

	if (argc != 4) {
		fprintf(stderr, "Error: Invalid number of arguments.\n");
		print_help(argv[0]);
		return EXIT_FAILURE;
	}

	value = strtod(argv[1], &endptr);
	if (*endptr != '\0') {
		fprintf(stderr, "Error: Invalid value '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	snprintf(current_currency, sizeof(current_currency), "%s", argv[2]);
	uppercase_string(current_currency);

	if (!is_supported_currency(current_currency)) {
		fprintf(stderr, "Error: Unsupported current currency code '%s'.\n",
		 current_currency);
		return EXIT_FAILURE;
	}

	snprintf(target_currencies_str, sizeof(target_currencies_str), "%s",
	 argv[3]);

	/* Fetch rates. Optimization: if multiple targets, fetch all. */
	if (strchr(target_currencies_str, ',') != NULL) {
		json = fetch_rates(current_currency, NULL);
	} else {
		uppercase_string(target_currencies_str);
		if (!is_supported_currency(target_currencies_str)) {
			fprintf(stderr, "Error: Unsupported current currency code '%s'.\n",
			 target_currencies_str);
			return EXIT_FAILURE;
		}
		json = fetch_rates(current_currency, target_currencies_str);
	}

	if (json == NULL) {
		fprintf(stderr, "Error: Failed to fetch exchange rates.\n");
		return EXIT_FAILURE;
	}

	/* Process target currencies list */
	current_pos = target_currencies_str;
	while (current_pos != NULL && *current_pos != '\0') {
		next_comma = strchr(current_pos, ',');
		if (next_comma != NULL) {
			*next_comma = '\0';
		}

		if (*current_pos != '\0') {
			double rate;
			double converted_value;
			uppercase_string(current_pos);
			if (!is_supported_currency(current_pos)) {
				fprintf(stderr,
				 "Error: Unsupported target currency code '%s'.\n",
				 current_pos);
			} else {
				rate = get_rate_for_currency(json, current_pos);
				if (rate >= 0) {
					converted_value = value * rate;
					printf("%.2f %s = %.2f %s\n", value, current_currency,
					 converted_value, current_pos);
				} else {
					fprintf(stderr,
					 "Error: Failed to get rate for '%s' from API.\n",
					 current_pos);
				}
			}
		}

		if (next_comma != NULL) {
			current_pos = next_comma + 1;
		} else {
			current_pos = NULL;
		}
	}

	cJSON_Delete(json);

	return EXIT_SUCCESS;
}
