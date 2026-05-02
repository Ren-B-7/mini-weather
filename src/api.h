#ifndef API_H
#define API_H

#include <cjson/cJSON.h>

/**
 * Fetches current weather data from Open-Meteo.
 * @param city The city name (used for geocoding lookup).
 * @param units Units: 'metric' or 'imperial'.
 * @return A cJSON object containing the weather forecast, or NULL on error.
 */
cJSON* fetch_weather(const char* city, const char* units);

/**
 * Fetches current weather data for multiple cities from Open-Meteo in parallel.
 * @param cities An array of city names.
 * @param num_cities The number of cities in the array.
 * @param units Units: 'metric' or 'imperial'.
 * @return An array of cJSON objects containing the weather forecast, or NULL on error.
 *         The caller is responsible for freeing the array and each cJSON object.
 */
cJSON** fetch_weather_multi(const char** cities, int num_cities, const char* units);

#endif /* API_H */
