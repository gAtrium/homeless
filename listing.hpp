#pragma once
#include <string>
#include <vector>
#include <fstream>
enum status {
    DISMISSED,
    UNCONTACTED,
    };

struct listing {
    std::string title;
    std::string price;
    std::string location;
    std::string numberofrooms;
    unsigned long long listingID;
    std::string url;
    std::string dismiss_reason;
    status _status;
    unsigned int tg_message_id;
};

void save_listings_to_file(std::string filename = "/etc/homeless");
std::vector<listing>* load_listings_from_file(std::string filename);
listing* check_if_listing_exists_liD(unsigned long listingID);
listing* check_if_listing_exists_miD(unsigned int messageID);
void add_listing(listing listing);
void dismiss_listing(unsigned int messageID, std::string reason);