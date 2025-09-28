#include <mariadb/conncpp.hpp>
#include <tgbot/tgbot.h>
#include <thread>

using namespace std;
using namespace TgBot;
using namespace sql;

enum class UserState { Idle, Word };

const int to_delete = 3;
unique_ptr<Connection> conn;
unordered_map<int64_t, UserState> userStates;

Bot bot(getenv("TOKEN"));

vector<string> getTextArguments(string text) {
  stringstream s(text);
  string t;
  vector<string> result;
  while (getline(s, t, ' ')) {
    result.push_back(t);
  }
  return result;
}

void addWord(int64_t chat_id, SQLString word, SQLString translation,
             int to_delete) {
  shared_ptr<PreparedStatement> stmnt(
      conn->prepareStatement("INSERT INTO test.Word VALUES (?, ?, ?, ?)"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->setString(3, translation);
  stmnt->setInt(4, to_delete);
  stmnt->executeUpdate();
}

bool existsWord(SQLString word) {
  shared_ptr<PreparedStatement> stmnt(conn->prepareStatement(
      "SELECT EXISTS (SELECT * FROM test.Word WHERE word = ?) as word_exists"));
  stmnt->setString(1, word);
  unique_ptr<ResultSet> res(stmnt->executeQuery());
  while (res->next()) {
      return res->getBoolean("word_exists");
  }
  return false;
}

unordered_map<int64_t, string> getActualWords() {
  shared_ptr<Statement> stmnt(conn->createStatement());
  unique_ptr<ResultSet> res(stmnt->executeQuery(
      "SELECT chat_id, MAX(to_delete), word FROM test.Word GROUP BY chat_id"));
  unordered_map<int64_t, string> result;
  while (res->next()) {
    result[res->getInt64("chat_id")] = res->getString("word");
  }
  return result;
}

void notifyTranslate() {
  while (true) {
    this_thread::sleep_for(chrono::seconds(1));
    unordered_map<int64_t, string> words = getActualWords();
    for (const auto& [chat_id, word] : words) {
      bot.getApi().sendMessage(chat_id, word);
      //todo: decrement to_delete count in word record
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

  Driver *driver = mariadb::get_driver_instance();
 SQLString url("jdbc:mariadb://localhost:3306/echobot_db");
  Properties properties({{"user", "echobot"}, {"password", "strong_password"}});
  conn = unique_ptr<Connection>(driver->connect(url, properties));
  if (!conn) {
    printf("Invalid database connection\n");
    exit(0);
  }

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
      //todo: filter arguments
      if (!existsWord(result[0])) {
        addWord(message->chat->id, result[0], result[1], to_delete);
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
