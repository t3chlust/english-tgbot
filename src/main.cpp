#include <thread>

#include <mariadb/conncpp.hpp>
#include <tgbot/tgbot.h>

#include "database.h"
#include "utils.h"

enum class UserState { Idle, Word };

const int to_delete = 3;

Database db;
std::unordered_map<int64_t, UserState> userStates;

TgBot::Bot bot(getenv("TOKEN"));

void notifyTranslate() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::vector<Word> words = db.getActualWords();
    for (const auto &word : words) {
      bot.getApi().sendMessage(word.chat_id,
                               "Переведите слово: " + word.word + "\n" +
                                   "Посмотреть перевод: ||" + word.translation +
                                   "||",
                               nullptr, nullptr, nullptr, "MarkdownV2");
      if (db.getWordDeletionCount(word.chat_id, word.word) <= 1) {
        db.deleteWord(word.chat_id, word.word);
      } else {
        db.incrementWordDeletionCount(word.chat_id, word.word);
      }
    }
  }
}

int main() {
  std::vector<TgBot::BotCommand::Ptr> commands;
  TgBot::BotCommand::Ptr cmdArray(new TgBot::BotCommand);
  cmdArray->command = "word";
  cmdArray->description = "добавить новое слово";
  commands.push_back(cmdArray);

  bot.getApi().setMyCommands(commands);

  bot.getEvents().onCommand("word", [](TgBot::Message::Ptr message) {
    userStates[message->chat->id] = UserState::Word;
    bot.getApi().sendMessage(
        message->chat->id,
        "Добавление слова. Пожалуйста, введите слово и его перевод.");
  });
  bot.getEvents().onAnyMessage([](TgBot::Message::Ptr message) {
    int64_t user = message->chat->id;
    if (userStates.find(user) == userStates.end()) {
      return;
    }
    UserState userState = userStates[user];
    switch (userState) {
    case UserState::Idle:
      break;
    case UserState::Word:
      std::vector<std::string> result = getTextArguments(message->text);
      if (result.size() < 2) {
        return;
      }
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

    std::thread th1(notifyTranslate);
    th1.detach();

    TgBot::TgLongPoll longPoll(bot);
    while (true) {
      printf("Long poll started\n");
      longPoll.start();
    }
  } catch (TgBot::TgException &e) {
    printf("error: %s\n", e.what());
  }
}
