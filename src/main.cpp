#include <mariadb/conncpp.hpp>
#include <tgbot/tgbot.h>
#include <thread>

#include "database.h"
#include "utils.h"

using namespace std;
using namespace TgBot;
using namespace sql;

enum class UserState { Idle, Word };

const int to_delete = 3;

Database db;
unordered_map<int64_t, UserState> userStates;

Bot bot(getenv("TOKEN"));

void notifyTranslate() {
  while (true) {
    this_thread::sleep_for(chrono::seconds(5));
    unordered_map<int64_t, string> words = db.getActualWords();
    for (const auto &[chat_id, word] : words) {
      bot.getApi().sendMessage(chat_id, "Переведите слово: " + word);
      if (db.getWordDeletionCount(chat_id, word) <= 1) {
        db.deleteWord(chat_id, word);
      } else {
        db.incrementWordDeletionCount(chat_id, word);
      }
    }
  }
}

int main() {
  vector<BotCommand::Ptr> commands;
  BotCommand::Ptr cmdArray(new BotCommand);
  cmdArray->command = "word";
  cmdArray->description = "добавить новое слово";
  commands.push_back(cmdArray);

  bot.getApi().setMyCommands(commands);

  bot.getEvents().onCommand("word", [](Message::Ptr message) {
    userStates[message->chat->id] = UserState::Word;
    bot.getApi().sendMessage(
        message->chat->id,
        "Добавление слова. Пожалуйста, введите слово и его перевод.");
  });
  bot.getEvents().onAnyMessage([](Message::Ptr message) {
    int64_t user = message->chat->id;
    if (userStates.find(user) == userStates.end()) {
      return;
    }
    UserState userState = userStates[user];
    switch (userState) {
    case UserState::Idle:
      break;
    case UserState::Word:
      vector<string> result = getTextArguments(message->text);
      if (result.size() < 2) {
        return;
      }
      // todo: filter arguments
      if (!db.existsWord(result[0])) {
        db.addWord(message->chat->id, result[0], result[1], to_delete);
        bot.getApi().sendMessage(user, "Слово добавлено.");
      } else {
        bot.getApi().sendMessage(user, "Слово уже существует!");
      }
      userStates[user] = UserState::Idle;
      break;
    }
  });

  try {
    printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
    bot.getApi().deleteWebhook();

    thread th1(notifyTranslate);
    th1.detach();

    TgLongPoll longPoll(bot);
    while (true) {
      printf("Long poll started\n");
      longPoll.start();
    }
  } catch (exception &e) {
    printf("error: %s\n", e.what());
  }
}
