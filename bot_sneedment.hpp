#pragma once
#include <string>
#include "listing.hpp"

void send_message_to_gc(std::string);
bool send_reply_to_gc(std::string, unsigned int);
bool send_listing_to_gc(listing);