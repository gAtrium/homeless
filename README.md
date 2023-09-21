# homeless
Sahibinden.com scraper telegram bot written in  C++

# Dependencies
* Curl
* TgBot-cpp
* CMake
* Nlohmann-json

# Usage
 * Set the `FC_DEV` environment variable to your group ID or your user ID.
 * Get your Bot API token and set it to TGBOT_TOKEN env variable. 
 * create a file in /etc/ called homeless, chown it.

**It won't start scraping automatically.** 
 * Set your query text in the bot
 * Grab your favorite browser and Load up the website
 * copy the first network request as Curl.
 * Send the curl command to the bot, it should start scraping the website every 10 minutes (interval is changeable.)
I recommend screwing around and finding out. Just send /help to the bot.
