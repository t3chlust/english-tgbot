#include <mariadb/conncpp.hpp>
#include <tgbot/tgbot.h>

#include "database.h"

using namespace std;
using namespace TgBot;
using namespace sql;

Database::Database() {
  Driver *driver = mariadb::get_driver_instance();
  SQLString url("jdbc:mariadb://localhost:3306/echobot_db");
  Properties properties({{"user", "echobot"}, {"password", "strong_password"}});
  conn = unique_ptr<Connection>(driver->connect(url, properties));
  if (!conn) {
    printf("Invalid database connection\n");
    exit(0);
  }
}

void Database::addWord(int64_t chat_id, SQLString word, SQLString translation,
                       int to_delete) {
  shared_ptr<PreparedStatement> stmnt(
      conn->prepareStatement("INSERT INTO test.Word VALUES (?, ?, ?, ?)"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->setString(3, translation);
  stmnt->setInt(4, to_delete);
  stmnt->executeUpdate();
}

int Database::getWordDeletionCount(int64_t chat_id, SQLString word) {
  shared_ptr<PreparedStatement> stmnt(conn->prepareStatement(
      "SELECT to_delete FROM test.Word WHERE chat_id = ? AND word = ?"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  unique_ptr<ResultSet> res(stmnt->executeQuery());
  while (res->next()) {
    return res->getInt("to_delete");
  }
  return -1;
}

void Database::incrementWordDeletionCount(int64_t chat_id, SQLString word) {
  shared_ptr<PreparedStatement> stmnt(
      conn->prepareStatement("UPDATE test.Word SET to_delete = to_delete "
                             "- 1 WHERE chat_id = ? AND word = ?"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->executeQuery();
}

void Database::deleteWord(int64_t chat_id, SQLString word) {
  shared_ptr<PreparedStatement> stmnt(conn->prepareStatement(
      "DELETE FROM test.Word WHERE chat_id = ? AND word = ?"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->executeUpdate();
}

bool Database::existsWord(SQLString word) {
  shared_ptr<PreparedStatement> stmnt(
      conn->prepareStatement("SELECT EXISTS (SELECT * FROM test.Word "
                             "WHERE word = ?) as word_exists"));
  stmnt->setString(1, word);
  unique_ptr<ResultSet> res(stmnt->executeQuery());
  while (res->next()) {
    return res->getBoolean("word_exists");
  }
  return false;
}

unordered_map<int64_t, string> Database::getActualWords() {
  shared_ptr<Statement> stmnt(conn->createStatement());
  unique_ptr<ResultSet> res(
      stmnt->executeQuery("SELECT chat_id, MAX(to_delete), word FROM "
                          "test.Word GROUP BY chat_id"));
  unordered_map<int64_t, string> result;
  while (res->next()) {
    result[res->getInt64("chat_id")] = res->getString("word");
  }
  return result;
}
