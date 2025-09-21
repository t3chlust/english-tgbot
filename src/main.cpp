#include <mariadb/conncpp.hpp>
#include <tgbot/tgbot.h>

using namespace std;
using namespace TgBot;
using namespace sql;

const int to_delete = 3;
unique_ptr<Connection> conn;

vector<string> getTextArguments(string text) {
  stringstream s(text);
  string t;
  vector<string> result;
  while (getline(s, t, ' ')) {
    result.push_back(t);
  }
  return result;
}

void addWord(SQLString word,
             SQLString translation, int to_delete) {
  shared_ptr<PreparedStatement> stmnt(
      conn->prepareStatement("INSERT INTO test.Word (word, translation, "
                             "to_delete) VALUES (?, ?, ?)"));
  stmnt->setString(1, word);
  stmnt->setString(2, translation);
  stmnt->setInt(3, to_delete);
  stmnt->executeUpdate();
}

bool existsWord(SQLString word) {
  shared_ptr<PreparedStatement> stmnt(conn->prepareStatement(
      "SELECT EXISTS (SELECT * FROM test.Word WHERE word = ?) as word_exists"));
  stmnt->setString(1, word);
  unique_ptr<ResultSet> res(stmnt->executeQuery());
  res->next();
  return res->getBoolean("word_exists");
}

int main() {
  string token = getenv("TOKEN");
  Bot bot(token);

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

  bot.getEvents().onCommand("word", [&bot](Message::Ptr message) {
    bot.getApi().sendMessage(
        message->chat->id,
        "Добавление слова. Пожалуйста, введите слово и его перевод.");
  });
  bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
    vector<string> result = getTextArguments(message->text);
    if (result.size() != 2) {
      return;
    }
    if (!existsWord(result[0])) {
      addWord(result[0], result[1], to_delete);
      bot.getApi().sendMessage(message->chat->id, "Слово добавлено.");
    } else {
      bot.getApi().sendMessage(message->chat->id, "Слово уже существует!");
    }
  });

  try {
    printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
    bot.getApi().deleteWebhook();

    TgLongPoll longPoll(bot);
    while (true) {
      printf("Long poll started\n");
      longPoll.start();
    }
  } catch (exception &e) {
    printf("error: %s\n", e.what());
  }
}
