#include "listing.hpp"
#include "scraper.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <ostream>

std::vector<listing *> listings;
std::map<unsigned long, listing *> listings_map;
std::map<unsigned int, listing *> listings_map_mid;
const char MAGIC_BYTES[] = {0x48, 0x4F, 0x4D, 0x45, 0x4C, 0x45, 0x53, 0x53};

bool MAGIC_BYTE_READ(std::ifstream &file) {
  char c[sizeof(MAGIC_BYTES)];
  try {
    file.read(c, sizeof(c));
  } catch (std::exception e) {
    std::cerr << "MAGIC ERROR: " << e.what() << std::endl;
    char nc[sizeof(MAGIC_BYTES) +1];
    for (int i = 0; i < sizeof(MAGIC_BYTES); i++) {
      nc[i] = c[i];
    }
    nc[sizeof(MAGIC_BYTES)] = '\0';
    std::cerr << nc << std::endl;
    std::cerr.flush();
    return false; 
  }
  for (int i = 0; i < sizeof(MAGIC_BYTES); i++) {
    if (c[i] != MAGIC_BYTES[i]) {
      return false;
    }
  }
  return true;
}
void MAGIC_BYTE_WRITE(std::ofstream &file) {
  file.write(MAGIC_BYTES, sizeof(MAGIC_BYTES));
}

std::string read_until_null(std::fstream &file) {
  std::string ret = "";
  char c;
  while (file.read(&c, sizeof(c)) && c != '\0') {
    ret += c;
  }
  return ret;
}
void write_string_to_file(std::string &str, std::ofstream &file) {
  auto size = str.size();
  file.write((char *)&size, sizeof(size));
  file.write(str.c_str(), size);
};

std::string read_string_from_file(std::ifstream &file) {
  // read the size of the string
  size_t size;
  file.read((char *)&size, sizeof(size));
  // read the string
  char *buffer = new char[size];
  file.read(buffer, size);
  std::string ret(buffer, size);
  delete[] buffer;
  //std::cout << "READ_STRING: " << ret << std::endl;
  //std::cout.flush();
  return ret;
}

std::vector<listing> *load_listings_from_file(std::string filename) {
  // open the file as binary
  std::ifstream file(filename, std::ios::binary);
  // try reading the magic byte
  if (!MAGIC_BYTE_READ(file)) {
    std::cerr << "File header magic byte is wrong, possibly first-time "
                 "execution. Aborting"
              << std::endl;
    return nullptr;
  }
  // create a vector to store the listings
  std::vector<listing> *listings = new std::vector<listing>;
  // read the entry count
  size_t entry_count;
  // try to read the entry count
  try {
    file.read((char *)&entry_count, sizeof(entry_count));
  } catch (std::exception e) {
    std::cerr
        << "Could not read entry count, possibly first-time execution. Aborting"
        << std::endl;
    return nullptr;
  }
  // read the refresh interval setting as string until null byte
  set_interval(read_string_from_file(file));
  set_min_price(read_string_from_file(file));
  set_max_price(read_string_from_file(file));
  set_base_url(read_string_from_file(file));
  set_query_text(read_string_from_file(file));
  // iterate through the entries
  for (int i = 0; i < entry_count; i++) {
    // check the magic byte
    if (!MAGIC_BYTE_READ(file)) {
      // magic byte is wrong, abort
      std::cerr << "Magic byte is wrong, aborting" << std::endl;
      return nullptr;
    }
    // read the title
    std::string title = read_string_from_file(file);
    // read the price
    std::string price = read_string_from_file(file);

    std::string location = read_string_from_file(file);
    std::string numberofrooms = read_string_from_file(file);
    // read the listingID
    unsigned long long listingID;
    file.read((char *)&listingID, sizeof(listingID));
    // read the url
    std::string url = read_string_from_file(file);
    // read the dismiss_reason
    std::string dismiss_reason = read_string_from_file(file);
    // read the status
    status __status;
    file.read((char *)&__status, sizeof(status));
    // read the tg_message_id
    unsigned int tg_message_id;
    file.read((char *)&tg_message_id, sizeof(tg_message_id));
    // create a listing object
    listing listing = {title,          price,     location,
                       numberofrooms,  listingID, url,
                       dismiss_reason, __status,  tg_message_id};
    add_listing(listing);
  }
  size_t nono_words_size;
  file.read((char *)&nono_words_size, sizeof(nono_words_size));
  for (int i = 0; i < nono_words_size; i++) {
    // read until null byte
    std::string no_word;
    no_word = read_string_from_file(file);
    // get the amount of bytes read from file
    add_filters_to_title(no_word);
  }
  // cout ttest to see if the no no words are loaded
  return listings;
}

void save_listings_to_file(std::string filename) {
  // open the file
  std::ofstream file(filename, std::ios::binary);
  // write the magic byte
  MAGIC_BYTE_WRITE(file);
  // write the entry count to file
  size_t entry_count = listings.size();
  file.write((char *)&entry_count, sizeof(entry_count));
  // write the refresh interval setting as string until null byte
  std::string refresh_interval = get_interval();
  write_string_to_file(refresh_interval, file);
  // write the min price setting as string until null byte
  std::string min_price = get_min_price();
  write_string_to_file(min_price, file);
  // write the max price setting as string until null byte
  std::string max_price = get_max_price();
  write_string_to_file(max_price, file);
  // write the base url setting as string until null byte
  std::string base_url = get_base_url();
  write_string_to_file(base_url, file);
  // write the query text setting as string until null byte
  std::string query_text = get_query_text();
  write_string_to_file(query_text, file);
  // iterate through the listings
  for (int i = 0; i < listings.size(); i++) {
    // write the magic byte
    MAGIC_BYTE_WRITE(file);
    // write the title
    write_string_to_file(listings.at(i)->title, file);
    // write the price
    write_string_to_file(listings.at(i)->price, file);
    // write the location
    write_string_to_file(listings.at(i)->location, file);
    write_string_to_file(listings.at(i)->numberofrooms, file);
    // write the listingID
    file.write((char *)&listings.at(i)->listingID,
               sizeof(listings.at(i)->listingID));
    // write the url
    write_string_to_file(listings.at(i)->url, file);
    // write the dismiss_reason
    write_string_to_file(listings.at(i)->dismiss_reason, file);
    // write the status
    file.write((char *)&listings.at(i)->_status,
               sizeof(listings.at(i)->_status));
    // write the tg_message_id
    file.write((char *)&listings.at(i)->tg_message_id,
               sizeof(listings.at(i)->tg_message_id));
  }
  // get the no no words
  auto nono_words = get_nono_words_vector();
  size_t nono_words_size = nono_words->size();
  // write the no no words size
  file.write((char *)&nono_words_size, sizeof(nono_words_size));
  // iterate through the no no words
  for (int i = 0; i < nono_words->size(); i++) {
    write_string_to_file(nono_words->at(i), file);
  }
  // close the file
  file.close();
}

listing *check_if_listing_exists_liD(unsigned long listingID) {
  if (listings_map.find(listingID) != listings_map.end()) {
    return listings_map[listingID];
  }
  return nullptr;
}
listing *check_if_listing_exists_miD(unsigned int messageID) {
  if (listings_map_mid.find(messageID) != listings_map_mid.end()) {
    return listings_map_mid[messageID];
  }
  return nullptr;
}

void dissmiss_listing(unsigned long listingID, std::string reason) {
  listing *listing = check_if_listing_exists_liD(listingID);
  if (listing != nullptr) {
    listing->_status = DISMISSED;
    listing->dismiss_reason = reason;
  }
}

void add_listing(listing listing) {
  struct listing *list = new struct listing();
  list->title = listing.title;
  list->location = listing.location;
  list->numberofrooms = listing.numberofrooms;
  list->listingID = listing.listingID;
  list->_status = listing._status;
  list->dismiss_reason = listing.dismiss_reason;
  list->price = listing.price;
  list->tg_message_id = listing.tg_message_id;

  listings.push_back(list);
  listings_map[listing.listingID] = list;
  listings_map_mid[listing.tg_message_id] = list;
}