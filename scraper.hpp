#pragma once
#include <string>
#include <vector>
#include <thread>
std::string set_curl_command(std::string command);
std::string get_curl_command();
std::string set_min_price(std::string price);
std::string get_min_price();
std::string set_max_price(std::string price);
std::string get_max_price();
std::string set_interval(std::string interval);
bool get_run_thread();
std::string get_interval();
std::string set_base_url(std::string url);
std::string get_base_url();
std::string set_query_text(std::string query);
std::string get_query_text();
std::string add_filters_to_title(std::string nonoword);
std::string remove_filters_from_title(std::string nonoword);
std::string get_nono_words();
std::vector<std::string>* get_nono_words_vector();
void startscraper();
void scraper_main_thread();
void set_scraper_thread(std::thread* thread);
int parse_page(std::string response);