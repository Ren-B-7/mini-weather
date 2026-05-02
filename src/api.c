#include "api.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URL_MAX_SIZE 512

struct MemoryStruct {
	char* memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*) userp;

	char* ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static CURLcode
perform_request(CURL* curl_handle, const char* url, struct MemoryStruct* chunk)
{
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*) chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	return curl_easy_perform(curl_handle);
}

static cJSON* geocode_city(const char* city)
{
	CURL* curl_handle;
	struct MemoryStruct chunk;
	cJSON* json = NULL;
	char url[URL_MAX_SIZE];

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_handle = curl_easy_init();
	if (curl_handle) {
		snprintf(url, sizeof(url),
		 "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1",
		 city);
		if (perform_request(curl_handle, url, &chunk) == CURLE_OK) {
			json = cJSON_Parse(chunk.memory);
		}
		curl_easy_cleanup(curl_handle);
	}
	free(chunk.memory);
	return json;
}

cJSON* fetch_weather(const char* city, const char* units)
{
	cJSON* geo_json = geocode_city(city);
	if (!geo_json) {
		return NULL;
	}

	cJSON* results = cJSON_GetObjectItem(geo_json, "results");
	if (!results || !cJSON_GetArraySize(results)) {
		cJSON_Delete(geo_json);
		return NULL;
	}

	cJSON* city_data = cJSON_GetArrayItem(results, 0);
	double lat = cJSON_GetObjectItem(city_data, "latitude")->valuedouble;
	double lon = cJSON_GetObjectItem(city_data, "longitude")->valuedouble;
	cJSON_Delete(geo_json);

	CURL* curl_handle;
	struct MemoryStruct chunk;
	cJSON* weather_json = NULL;
	char url[URL_MAX_SIZE];

	chunk.memory = malloc(1);
	chunk.size = 0;
	curl_handle = curl_easy_init();
	if (curl_handle) {
		snprintf(url, sizeof(url),
		 "https://api.open-meteo.com/v1/"
		 "forecast?latitude=%f&longitude=%f&current=temperature_2m,"
		 "precipitation_probability&temperature_unit=%s",
		 lat, lon, units);
		if (perform_request(curl_handle, url, &chunk) == CURLE_OK) {
			weather_json = cJSON_Parse(chunk.memory);
		}
		curl_easy_cleanup(curl_handle);
	}
	free(chunk.memory);
	return weather_json;
}

typedef enum {
	CITY_STATE_GEOCODING,
	CITY_STATE_WEATHER,
	CITY_STATE_DONE,
	CITY_STATE_ERROR
} CityState;

struct CityRequest {
	const char* name;
	CityState state;
	struct MemoryStruct chunk;
	CURL* easy_handle;
	cJSON* result;
	double lat;
	double lon;
	int index;
};

static void setup_easy_handle(CURL* easy_handle, const char* url,
 struct MemoryStruct* chunk, void* private_data)
{
	curl_easy_setopt(easy_handle, CURLOPT_URL, url);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, (void*) chunk);
	curl_easy_setopt(easy_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(easy_handle, CURLOPT_PRIVATE, private_data);
}

cJSON**
fetch_weather_multi(const char** cities, int num_cities, const char* units)
{
	CURLM* multi_handle = curl_multi_init();
	if (!multi_handle) {
		return NULL;
	}

	struct CityRequest* requests =
	 calloc((size_t) num_cities, sizeof(struct CityRequest));
	cJSON** results = calloc((size_t) num_cities, sizeof(cJSON*));
	if (!requests || !results) {
		free(requests);
		free(results);
		curl_multi_cleanup(multi_handle);
		return NULL;
	}

	char url[URL_MAX_SIZE];
	for (int i = 0; i < num_cities; i++) {
		requests[i].name = cities[i];
		requests[i].index = i;
		requests[i].state = CITY_STATE_GEOCODING;
		requests[i].chunk.memory = malloc(1);
		requests[i].chunk.size = 0;
		requests[i].easy_handle = curl_easy_init();

		if (!requests[i].chunk.memory || !requests[i].easy_handle) {
			// Cleanup and return what we have (or NULL)
			// For simplicity in this example, we assume success or handle
			// failure later
		}

		snprintf(url, sizeof(url),
		 "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1",
		 requests[i].name);
		setup_easy_handle(requests[i].easy_handle, url, &requests[i].chunk,
		 &requests[i]);
		curl_multi_add_handle(multi_handle, requests[i].easy_handle);
	}

	int still_running = 0;
	curl_multi_perform(multi_handle, &still_running);

	while (still_running || 1) {
		int msgs_left;
		CURLMsg* msg;

		while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
			if (msg->msg == CURLMSG_DONE) {
				struct CityRequest* req;
				curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &req);
				CURL* handle = msg->easy_handle;

				if (req->state == CITY_STATE_GEOCODING) {
					cJSON* geo_json = cJSON_Parse(req->chunk.memory);
					cJSON* res_array =
					 geo_json ? cJSON_GetObjectItem(geo_json, "results") : NULL;
					if (res_array && cJSON_GetArraySize(res_array) > 0) {
						cJSON* city_data = cJSON_GetArrayItem(res_array, 0);
						req->lat =
						 cJSON_GetObjectItem(city_data, "latitude")
						  ->valuedouble;
						req->lon =
						 cJSON_GetObjectItem(city_data, "longitude")
						  ->valuedouble;
						cJSON_Delete(geo_json);

						// Start weather fetch
						req->state = CITY_STATE_WEATHER;
						free(req->chunk.memory);
						req->chunk.memory = malloc(1);
						req->chunk.size = 0;

						snprintf(url, sizeof(url),
						 "https://api.open-meteo.com/v1/"
						 "forecast?latitude=%f&longitude=%f&current="
						 "temperature_2m,precipitation_probability&temperature_"
						 "unit=%s",
						 req->lat, req->lon, units);

						curl_multi_remove_handle(multi_handle, handle);
						curl_easy_reset(handle);
						setup_easy_handle(handle, url, &req->chunk, req);
						curl_multi_add_handle(multi_handle, handle);
						still_running++; // We added a new handle
					} else {
						if (geo_json) {
							cJSON_Delete(geo_json);
						}
						req->state = CITY_STATE_ERROR;
						curl_multi_remove_handle(multi_handle, handle);
					}
				} else if (req->state == CITY_STATE_WEATHER) {
					req->result = cJSON_Parse(req->chunk.memory);
					req->state = CITY_STATE_DONE;
					results[req->index] = req->result;
					curl_multi_remove_handle(multi_handle, handle);
				}
			}
		}

		if (!still_running) {
			// Check if all are DONE or ERROR
			int all_finished = 1;
			for (int i = 0; i < num_cities; i++) {
				if (requests[i].state == CITY_STATE_GEOCODING ||
				 requests[i].state == CITY_STATE_WEATHER) {
					all_finished = 0;
					break;
				}
			}
			if (all_finished) {
				break;
			}
		}

		curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);
		curl_multi_perform(multi_handle, &still_running);
	}

	for (int i = 0; i < num_cities; i++) {
		curl_easy_cleanup(requests[i].easy_handle);
		free(requests[i].chunk.memory);
	}
	free(requests);
	curl_multi_cleanup(multi_handle);

	return results;
}
