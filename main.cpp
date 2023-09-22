#include "listing.hpp"
#include "scraper.hpp"
#include "tgbot/TgException.h"
#include "tgbot/types/Message.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <tgbot/tgbot.h>
#include "bot_sneedment.hpp"
#include <thread>
// TgBot::Bot* bot;
using namespace std;


long long fc_dev = 0;
void dump_message(TgBot::Message::Ptr message_ptr) {
  // dump every field of message object to stdout with null safety
  TgBot::Message message = *message_ptr;
  cout << "message_id: " << message.messageId << endl;
  cout << "from: " << message.from->firstName << endl;
  cout << "chat: " << message.chat->firstName << endl;
  cout << "date: " << message.date << endl;
  cout << "text: " << message.text << endl;
  cout << "entities: " << message.entities.size() << endl;
  // dump every entity
  for (int i = 0; i < message.entities.size(); i++) {
    cout << "entity " << i << endl;
    // cout << "type: " << message.entities[i]->type << endl;
    cout << "offset: " << message.entities[i]->offset << endl;
    cout << "length: " << message.entities[i]->length << endl;
    // cout << "url: " << message.entities[i]->url << endl;
    // cout << "user: " << message.entities[i]->user->firstName << endl;
  }
  cout << "audio: " << message.audio << endl;
  cout << "document: " << message.document << endl;
  cout << "photo: " << message.photo.size() << endl;
  cout << "sticker: " << message.sticker << endl;
  cout << "video: " << message.video << endl;
  cout << "voice: " << message.voice << endl;
  cout << "caption: " << message.caption << endl;
}
void load_default_settings(){
  set_min_price("3000");
  set_max_price("5000");
  add_filters_to_title("arkadaş");
  set_interval("10m");
  set_query_text("Bursa kiralık daire");
}
TgBot::Bot * _bot;
int _main() {
  std::ifstream s("parse_page_func.html",std::ios::in);
  std::stringstream buffer;
  buffer << s.rdbuf();
  parse_page(buffer.str());
  return 0;
}
int main() {
  //get FC_DEV from environment variables
  char * fc_dev_env = getenv("FC_DEV");
  if(fc_dev_env != nullptr) {
    fc_dev = std::stoll(fc_dev_env);
  }
  else {
    std::cout << "FC_DEV not found in environment variables, exiting..." << std::endl;
    return 1;
  }
  //get TGBOT_TOKEN from environment variables
  std::string tgbot_token = getenv("TGBOT_TOKEN");
  if(tgbot_token.size() < 2) {
    std::cout << "TGBOT_TOKEN not found in environment variables, exiting..." << std::endl;
    return 1;
  }

  //check if /etc/homeless exists, if not create it
  std::fstream file("/etc/homeless");
  if (!file.good()) {
    file.close(); 
    std::cout << "Creating /etc/homeless" << std::endl;
    std::ofstream outfile("/etc/homeless");
    outfile.close();
    load_default_settings();
  }
  else if(load_listings_from_file("/etc/homeless") == nullptr) {
    load_default_settings();
  }
  //load_default_settings();
  //save_listings_to_file();
  //exit(0);
  TgBot::Bot bot(tgbot_token);
    _bot = &bot;
  bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
    bot.getApi().sendVideo(message->chat->id, "BAACAgQAAx0CdJiREgACAkNlDKZJnDNgzy8ry2xt68pi1fK4rwACoxIAAr90aFCmvY3TUuGxvzAE");
    //bot.getApi().sendMessage(message->chat->id, "Hi!");
  });
  /*bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
    //check if we have a video
    if(message->video != nullptr) {
      bot.getApi().sendMessage(message->chat->id, message->video->fileId);
    }
  });*/
  bot.getEvents().onCommand("help", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    bot.getApi().sendMessage(
        message->chat->id,
        "Commands are:\n /startscrape #Starts scraping for "
        "adverts\n "
        "/curlupdate <curl command> #updates the curl command\n"
        "/updatequery <query> #updates the query\n"
        "/dismiss <reason> #reply to dismiss a listing with reason, reply dismiss again to remove the dismiss status.\n"
        "/set min/max <price> #set minimum or maximum price\n"
        "/set interval <intervalS/M/H> #set refresh interval\n"
        "/getsettings print current bot settings\n"
        "\n Bot may occasionally want you to send new curl commands, "
        "that's due to cloudflare sussing out.");
  });
  
  bot.getEvents().onCommand("dismiss", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    //check if replytomessage is not null
    if (message->replyToMessage == nullptr) {
      return;
    }
    //get the message id
    auto message_id = message->replyToMessage->messageId;
    auto listing = check_if_listing_exists_miD(message_id);
    if (listing == nullptr) {
      return;
    }
    //if the listing is already dismissed, clear dismiss status
    if (listing->_status == DISMISSED) {
      listing->_status = UNCONTACTED;
      listing->dismiss_reason = "";
      send_listing_to_gc(*listing);
      std::this_thread::sleep_for(std::chrono::seconds(2));
      bot.getApi().sendMessage(message->chat->id, "Listing undismissed", false,message->messageId);
      return;
    }
    //get the reason
    auto reason = message->text.substr(message->text.find("dismiss") + 8);
    listing->dismiss_reason = reason;
    listing->_status = DISMISSED;
    send_listing_to_gc(*listing);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    bot.getApi().sendMessage(message->chat->id, "Dismissed the listing", false,message->messageId); 
    });
  
  bot.getEvents().onCommand("set", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    auto text = message->text;
    //example text: "/set min 100"
    //check if we have "min" in the text
    if (text.find("min") != std::string::npos) {
      auto price = text.substr(text.find("min") + 4);
      bot.getApi().sendMessage(message->chat->id, "Setting minimum price to " + price);
      return;
    }
    else if (text.find("max") != std::string::npos) {
      auto price = text.substr(text.find("max") + 4);
      bot.getApi().sendMessage(message->chat->id, "Setting maximum price to " + price);
      return;
    }
    bot.getApi().sendMessage(message->chat->id, "Setting price");
  });
  
  bot.getEvents().onCommand("curlupdate", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    //check if we have message text in the pointer and if it's long enough
    if(!message) return;
    if(message->text.size() < 30) return;
    //take the text after the /curlupdate text 
    std::string curl_command = message->text.substr(message->text.find("curlupdate") + 11);
    bot.getApi().sendMessage(message->chat->id, set_curl_command(curl_command));
  });
  
  //get settings
  bot.getEvents().onCommand("getsettings", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    std::string settings = "Current settings:\n";
    settings += "Curl command: " + get_curl_command() + "\n";
    settings += "Min price: " + get_min_price() + "\n";
    settings += "Max price: " + get_max_price() + "\n";
    settings += "Interval: " + get_interval() + "\n";
    settings += "Base url: " + get_base_url() + "\n";
    settings += "Query text: " + get_query_text() + "\n";
    settings += "Nono words: " + get_nono_words() + "\n";
    settings += "Thread is running: ";
    if(get_run_thread())
      settings += "yes";
    else
      settings += "no";
    bot.getApi().sendMessage(message->chat->id, settings);
  });
  //start scraper
  bot.getEvents().onCommand("startscrape", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    bot.getApi().sendMessage(message->chat->id, "Starting/Stopping scraper");
    startscraper();
  });

  //update query
  bot.getEvents().onCommand("updatequery", [&bot](TgBot::Message::Ptr message) {
    if(message->chat->id != fc_dev) return;
    std::string query = message->text.substr(message->text.find("updatequery") + 11);
    bot.getApi().sendMessage(message->chat->id, set_query_text(query));
  });

  /*bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
    std::cout << "Got something" << std::endl;
    std::cout << message->chat->id;
    return;
    if (message->text.size() < 1) {
      return;
    }
    dump_message(message);
    printf("User wrote %s\n", message->text.c_str());
    if (StringTools::startsWith(message->text, "/start")) {
      return;
    }
    bot.getApi().sendMessage(message->chat->id,
                             "Your message is: " + message->text);
  });*/
  //start the scraper thread
  std::thread scraperthread(scraper_main_thread);
  try {
    printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
    TgBot::TgLongPoll longPoll(bot);
    while (true) {
      printf("Long poll started\n");
      longPoll.start();
    }
  } catch (TgBot::TgException &e) {
    printf("error: %s\n", e.what());
  }
  return 0;
}

void handle_ratelimit(TgBot::TgException &ex) {
  auto what = std::string(ex.what());
  if(what.find("Too Many") != what.npos) {
    auto n = what.find("retry after ") + 12;
    auto nn = std::stoul(what.substr(n));
    std::cout << "Hit a rate limit, waiting for " << nn << " seconds..." << std::endl;
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::seconds(nn+5));
  }
}
void send_message_to_gc(std::string message) {
  try {
    _bot->getApi().sendMessage(fc_dev, message);
  }
  catch(TgBot::TgException &ex) {
    handle_ratelimit(ex);
  }
}
bool send_reply_to_gc(std::string text, unsigned int to) {
  try {
    _bot->getApi().sendMessage(fc_dev, text,false,to);
  }
  catch (TgBot::TgException &ex){
    handle_ratelimit(ex);
    return false;
  }
  return true;
}
bool send_listing_to_gc(listing listing) {
  //check if we have already sent this listing
  std::string message = "Title: " + listing.title + "\n" + "Price: " + listing.price + '\n' + "Number of rooms: " + listing.numberofrooms + "\nLocation: " + listing.location +
                        "\n" + "https://www.sahibinden.com" +listing.url + "\n";
  //add dismiss status to the message
  if (listing._status == DISMISSED) {
    message += "Dismissed: " + listing.dismiss_reason;
  }
  else {
    message += "Status: Uncontacted";
  }
  if (check_if_listing_exists_liD(listing.listingID) != nullptr) {
    try {
      _bot->getApi().editMessageText(message, fc_dev, listing.tg_message_id);
    }
    catch (TgBot::TgException &ex) {
      handle_ratelimit(ex);
      return false;
    }
    return true;
  }
  TgBot::Message::Ptr tgmessage;
  try {
    tgmessage = _bot->getApi().sendMessage(fc_dev, message);
  }
  catch (TgBot::TgException &ex) {
    handle_ratelimit(ex);
    return false;
  }
  listing.tg_message_id = tgmessage->messageId;
  add_listing(listing);
  return true;
}


//we did this thing inside main
void dismiss_listing(unsigned long messageID, std::string reason){
  //check if we have already sent this listing
  auto listing = check_if_listing_exists_liD(messageID);
  if (listing == nullptr) {
    return;
  }
  listing->dismiss_reason = reason;
  listing->_status = DISMISSED;
  _bot->getApi().editMessageText("Dismissed", fc_dev, listing->tg_message_id);
}
