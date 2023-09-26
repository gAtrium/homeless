#include "scraper.hpp"
#include "bot_sneedment.hpp"
#include "listing.hpp"
#include "tgbot/TgException.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <curl/curl.h>
#include <curl/easy.h>
#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

std::string curl_command;
std::vector<std::string> curl_headers;
unsigned int min_price = 3000;
unsigned int max_price = 5000;
std::string query = "bursa+kiralik+daire";
std::string base_url = "https://www.sahibinden.com/kiralik-daire/"
                       "bursa?pagingSize=50&sorting=price_asc";
const std::string url_arg_minprice = "price_min";

const std::string url_arg_maxprice = "price_max";
const std::string url_arg_query = "query_text_mf";
const std::string url_force_50_entries = "pagingSize=50";
std::string final_url;
unsigned int interval = 10; // in seconds
struct curl_slist *headers = NULL;
CURL *curl;

int parse_page(std::string);
void build_final_url();

bool run_thread = false;
std::string set_curl_command(std::string command) {
  if (run_thread) {
    run_thread = false;
    sleep(3);
  }
  if (curl != nullptr) {
    curl_easy_cleanup(curl);
  }
  curl = curl_easy_init();
  // check if we have the headers initialized
  if (headers != nullptr) {
    // free the headers
    curl_slist_free_all(headers);
    headers = nullptr;
  }
  // check if the thread is running, if it does, stop it

  curl_command = command;
  curl_headers.clear();
  // find the first -H and discard everything before it
  if (command.find("-H") == std::string::npos) {
    return "Invalid curl command, no -H flag found";
  }
  command = command.substr(command.find("-H"));
  // split the command every -H and add each part to the headers
  while (command.find("-H '") != std::string::npos) {
    // find the first -H
    size_t first = command.find("-H '");
    // check if we have -H after this one and substr until that command
    auto next = command.find("-H '", first + 1);
    if (next != std::string::npos) {
      first += 2;
      next -= 4;
      auto s = command.substr(first + 2, next - first);
      std::cout << s << std::endl;
      curl_headers.push_back(s);
      headers = curl_slist_append(headers, s.c_str());
      // set the command to the rest of the string
      command = command.substr(next);
    } else {
      // we don't have -H after this one, substr until the end
      auto s = command.substr(first + 2);
      // find the first '
      auto start = s.find("'");
      // find the last '
      auto end = s.find_last_of("'");
      // substr between those two
      s = s.substr(start + 1, end - start - 1);
      curl_headers.push_back(s);
      headers = curl_slist_append(headers, s.c_str());
      command = "";
    }
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  startscraper();
  return "Parsed the cUrl command. Scraping should begin momentarily.";
}

std::string get_curl_command() {
  std::string s;
  // concat curl_header items into s and return it
  for (int i = 0; i < curl_headers.size(); i++) {
    s += curl_headers[i] + "\n";
  }
  return s;
}

std::string set_min_price(std::string price) {
  try {
    min_price = std::stoi(price);
    build_final_url();
    return "Set minimum price to " + price;
  } catch (std::invalid_argument) {
    return "Invalid price, cannot parse.";
  }
}
std::string get_min_price() { return std::to_string(min_price); }

std::string set_max_price(std::string price) {
  try {
    max_price = std::stoi(price);
    build_final_url();
    return "Set maximum price to " + price;
  } catch (std::invalid_argument) {
    return "Invalid price, cannot parse.";
  }
}
std::string get_max_price() { return std::to_string(max_price); }

std::string set_interval(std::string interval) {
  // input examples: 10s, 10m, 10h
  // check if the last character is s, m or h
  if (interval.back() != 's' && interval.back() != 'm' &&
      interval.back() != 'h') {
    return "Invalid interval, must end with s, m or h";
  }
  // get the number
  int number = std::stoi(interval.substr(0, interval.size() - 1));
  // check if the number is valid
  if (number < 1) {
    return "Invalid interval, must be greater than 0";
  }
  // set the interval
  char unit = interval.back();
  switch (unit) {
  case 's':
    ::interval = number;
    break;
  case 'm':
    ::interval = number * 60;
    break;
  case 'h':
    ::interval = number * 60 * 60;
    break;
  }
  return "Set interval to " + interval;
}
std::string get_interval() {
  // convert the interval to a string
  std::string interval;
  if (::interval < 60) {
    interval = std::to_string(::interval) + "s";
  } else if (::interval < 60 * 60) {
    interval = std::to_string(::interval / 60) + "m";
  } else {
    interval = std::to_string(::interval / 60 / 60) + "h";
  }
  return interval;
}
std::string set_base_url(std::string url) {
  base_url = url;
  return "Set base url to " + url;
}
std::string get_base_url() { return base_url; }
bool get_run_thread() { return run_thread; };
std::string set_query_text(std::string query) {
  // replace spaces with + and set it to ::query
  std::replace(query.begin(), query.end(), ' ', '+');
  ::query = query;
  build_final_url();
  return "Set query text to " + query;
}
std::string get_query_text() { return query; }

void build_final_url() {
  // build the final url
  final_url = base_url;
  // check if we have a min price
  if (min_price > 0) {
    final_url += "&" + url_arg_minprice + "=" + std::to_string(min_price);
  }
  // check if we have a max price
  if (max_price > 0) {
    final_url += "&" + url_arg_maxprice + "=" + std::to_string(max_price);
  }
  // check if we have a query
  if (query.size() > 0) {
    final_url += "&" + url_arg_query + "=" + query;
  }
  // return final_url;
}
// Callback function to write the response body to a string
size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

int total_pages = 0;
const std::string url_arg_pageOffset = "pagingOffset";
int parse_page_amount(std::string response, bool isFirst);
bool did_something = false;
void scraper_main_thread() {
  // init curl
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  // set the headers
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  while (true) {
    if (!run_thread) {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      continue;
    }
    if (final_url.size() < 1) {
      send_message_to_gc(
          "final URL is empty. Missing details. Please review your settings, "
          "run /confirmurl and try running /startscrape again.");
      run_thread = false;
      continue;
    };
    if (curl_command.size() < 1 || curl == nullptr) {
      send_message_to_gc(
          "curl command is empty and/or curl is not initialized. I need a "
          "working CF session to scrape. Send me a CF cUrl command via "
          "/curlupdate and try running /startscrape again.");
      run_thread = false;
      continue;
    };

    int total_pages = 0;
    int page_offset = 0;
    do {
      // perform the curl request and get the response
      std::string response;
      auto _url = final_url + "&" + url_arg_pageOffset + "=" +
                  std::to_string(page_offset);
      curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      CURLcode res;
      try {
        res = curl_easy_perform(curl);
      } catch (std::exception &e) {
        send_message_to_gc("curl_easy_perform() failed: " +
                           std::string(curl_easy_strerror(res)));
        run_thread = false;
        continue;
      }
      if (res != CURLE_OK) {
        send_message_to_gc("curl_easy_perform() failed: " +
                           std::string(curl_easy_strerror(res)));
        run_thread = false;
        continue;
      }
      // parse the response
      // check if we tripped the cloudflare check
      if (response.find("Just a moment") != std::string::npos) {
        send_message_to_gc(
            "Cloudflare blocked me. Please send me a new cUrl command via "
            "/curlupdate and try running /startscrape again.");
        run_thread = false;
        continue;
      }
      int _total_pages = 0;
      try {
        _total_pages = parse_page_amount(
            response,
            ((page_offset == 0 || page_offset == (total_pages - 1) * 50)
                 ? true
                 : false));
      } catch (std::exception &e) {
        send_message_to_gc(
            "Failed to parse total amount of pages. Something is wrong.");
        std::cout << response << std::endl;
        run_thread = false;
      }
      if (!run_thread)
        break;
      if (total_pages != 0 && _total_pages != total_pages) {
        break; // we'll parse it again later, something drastic has happened.
      }
      if (total_pages == 0) {
        total_pages = _total_pages;
      }
      std::this_thread::sleep_for(std::chrono::seconds(
          1)); // Go ahead, find all the sleep calls here I dare you.
      int parsed_in_page = parse_page(response);
      if (parsed_in_page == 0) {
        send_message_to_gc("Failed to parse any listings. Stopping thread.");
        run_thread = false;
      } else if (parsed_in_page < 50) {
        // we have less than 50 listings, we're done here
        break;
      }
      page_offset += 50;
    } while (page_offset < total_pages * 50);
    if (did_something) {
      save_listings_to_file();
      did_something = false;
    }
    int slept_secs = 0;
    while (slept_secs < interval) {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      if (!run_thread) {
        break; // killing the wait mid-sleep to stop thread.
      }
      slept_secs+=2;
    }
  }
}

int parse_page_amount(std::string response, bool isFirst) {
  // get how many pages we have from how many li elements present inside <ul
  // class="pageNaviButtons"> get the position of the first <ul
  // class="pageNaviButtons">
  auto page_navi_buttons = response.find("<ul class=\"pageNaviButtons\">");
  if (page_navi_buttons == response.npos) {
    return 0;
  }
  // find the closing ul tag after that
  auto closing_ul = response.find("</ul>", page_navi_buttons);
  // get the substring between those two
  auto page_navi_buttons_html =
      response.substr(page_navi_buttons, closing_ul - page_navi_buttons);
  // std::ofstream i("page_amount.html", std::ofstream::out |
  // std::ofstream::trunc); i << page_navi_buttons_html; i.close();
  int li_count = 0;
  size_t last_li_index;
  // count how many <li> tags we have
  while ((last_li_index = page_navi_buttons_html.find("<li>", last_li_index)) !=
         std::string::npos) {
    li_count++;
    last_li_index++;
  }
  // we have the amount of pages
  return li_count - (isFirst ? 1 : 2);
}
std::vector<std::string> no_no_words;

std::string add_filters_to_title(std::string nonoword) {
  std::transform(nonoword.begin(), nonoword.end(), nonoword.begin(), ::tolower);
  if (std::find(no_no_words.begin(), no_no_words.end(), nonoword) !=
      no_no_words.end()) {
    return "Already have this word in the list.";
  }
  // add it to the list
  no_no_words.push_back(nonoword);
  return "Added " + nonoword + " to the no no list.";
};

std::string remove_filters_from_title(std::string nonoword) {
  std::transform(nonoword.begin(), nonoword.end(), nonoword.begin(), ::tolower);
  auto it = std::find(no_no_words.begin(), no_no_words.end(), nonoword);
  if (it == no_no_words.end()) {
    return "Could not find this word in the list.";
  }
  // remove it from the list
  no_no_words.erase(it);
  return "Removed " + nonoword + " from the no no list.";
};

std::string get_nono_words() {
  std::string words;
  for (int i = 0; i < no_no_words.size(); i++) {
    words += no_no_words[i] + "\n";
  }
  return words;
}
std::vector<std::string> *get_nono_words_vector() { return &no_no_words; }
bool check_if_listing_is_valid(std::string title) {
  // check if the title contains any of the no no words
  std::transform(title.begin(), title.end(), title.begin(), ::tolower);
  for (int i = 0; i < no_no_words.size(); i++) {
    if (title.find(no_no_words[i]) != std::string::npos) {
      return false;
    }
  }
  return true;
}
std::thread *scraper_thread;
void set_scraper_thread(std::thread *thread) { scraper_thread = thread; }

int parse_page(std::string response) {
  int listing_count;
  // std::ofstream i("parse_page_func.html", std::ofstream::out |
  // std::ofstream::trunc); i << response; i.close();
  //  lucky for us the listings in the page are given at the end of document as
  //  a JSON. find the first <script type="application/ld+json">
  const std::string start_tag = "<script type=\"application/ld+json\">";
  std::cout << start_tag << std::endl << response.size() << std::endl;
  auto json_start = response.find(start_tag);
  json_start = response.find(start_tag, json_start + 10);
  // find the end script tag
  auto json_end = response.find("</script>", json_start);
  // std::ofstream i("parse_page_func_what.html", std::ofstream::out |
  // std::ofstream::trunc); i << response.substr(json_start); i.close();
  if (json_start == std::string::npos || json_end == std::string::npos) {
    std::cout << "fuck" << std::endl;
    exit(-1);
    // we didn't find the script tags, something is wrong
    send_message_to_gc(
        "Could not find the script tags in the response. Something is wrong.");
    return 0;
  }
  // get the substring between those two
  auto json = response.substr(json_start, json_end - json_start);
  // remove the script tags
  // take a substr of the json without the first 34 characters and without the
  // last 9 characters
  json = json.substr(36, json.size() - 36);
  // std::ofstream j("parse_page_func_json.html", std::ofstream::out |
  // std::ofstream::trunc); j << json; j.close();
  //  now we have a valid json, parse it
  // output json to a text file, override if present
  // std::ofstream json_file;
  // json_file.open("json.txt", std::ofstream::out | std::ofstream::trunc);
  // json_file << json;
  // json_file.close();
  auto json_obj = nlohmann::json::parse(json);
  // json format:
  /*
  [
      {
      "@context": "https://schema.org/",
      "@type": [
          "SingleFamilyResidence",
          "Product"
      ],
      "name": "Bayan ev arkadasi ariyorum", //Benefits of being a woman in
  Turkey: A case study
  "numberOfRooms": "2+1", "address": {
          "@type": "PostalAddress",
          "addressLocality": "Nilüfer",
          "addressRegion": "Bursa"
      },
      "floorSize": {
          "@type": "QuantitativeValue",
          "value": "100"
      },
      "url":
  "/ilan/emlak-konut-kiralik-bayan-ev-arkadasi-ariyorum-1123991505/detay",
      "photo": "https://i0.shbdn.com/photos/99/15/05/x5_11239915050ci.jpg",
      "offers": {
          "@type": "Offer",
          "price": "3000",
          "priceCurrency": "TRY",
          "availability": "https://schema.org/InStock"
      }
  },
  ]
  */
  // get amount of elements in root array
  listing_count = json_obj.size();
  bool crashed = false;
  // iterate over each element
  for (int i = 0; i < listing_count; i++) {
    // get the listing object
    auto listing_obj = json_obj[i];
    std::string title = listing_obj["name"];
    if (!check_if_listing_is_valid(title)) {
      continue; // good riddance.
    }
    // get the url
    std::string url = listing_obj["url"];
    std::string numrooms = listing_obj["numberOfRooms"];
    std::string location = listing_obj["address"]["addressLocality"];
    // get the listingID
    // split the url by - and get the last element
    unsigned long listingID = std::stoul(
        url.substr(url.find_last_of('-') + 1).substr(0, url.find_last_of('/')));
    // check if we have it in our listings
    std::string price = listing_obj["offers"]["price"];
    listing *ex_listing = check_if_listing_exists_liD(listingID);
    if (ex_listing != nullptr) {
      // check if the price differs from what we have. If it does, update it
      if (price != ex_listing->price) {
        bool succ = send_reply_to_gc("Price action:" + ex_listing->price +
                                         " -> " + price,
                                     ex_listing->tg_message_id);
        // edit the message
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ex_listing->price = price;
        succ = succ && send_listing_to_gc(*ex_listing);
        did_something = true;
        if (!succ) {
          crashed = true;
          break;
        }
      }
      continue;
    }
    listing listing = {title,     price, location, numrooms,
                       listingID, url,   "",       UNCONTACTED};
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    send_listing_to_gc(listing); // this also handles updating the listings map
                                 // and editing the sent message.
    did_something = true;
  }
  return listing_count;
}
void startscraper() { run_thread = !run_thread; }