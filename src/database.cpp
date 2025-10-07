#include <mariadb/conncpp.hpp>
#include <tgbot/tgbot.h>

#include "utils.h"
#include "database.h"

Database::Database() {
  sql::Driver *driver = sql::mariadb::get_driver_instance();
  sql::SQLString url("jdbc:mariadb://localhost:3306/echobot_db");
  sql::Properties properties(
      {{"user", "echobot"}, {"password", "strong_password"}});
  conn = std::unique_ptr<sql::Connection>(driver->connect(url, properties));
  if (!conn) {
    printf("Invalid database connection\n");
    exit(0);
  }
}

void Database::addWord(int64_t chat_id, sql::SQLString word,
                       sql::SQLString translation, int to_delete) {
  std::shared_ptr<sql::PreparedStatement> stmnt(
      conn->prepareStatement("INSERT INTO test.Word VALUES (?, ?, ?, ?)"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->setString(3, translation);
  stmnt->setInt(4, to_delete);
  stmnt->executeUpdate();
}

int Database::getWordDeletionCount(int64_t chat_id, sql::SQLString word) {
  std::shared_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement(
      "SELECT to_delete FROM test.Word WHERE chat_id = ? AND word = ?"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
  while (res->next()) {
    return res->getInt("to_delete");
  }
  return -1;
}

void Database::incrementWordDeletionCount(int64_t chat_id,
                                          sql::SQLString word) {
  std::shared_ptr<sql::PreparedStatement> stmnt(
      conn->prepareStatement("UPDATE test.Word SET to_delete = to_delete "
                             "- 1 WHERE chat_id = ? AND word = ?"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->executeQuery();
}

void Database::deleteWord(int64_t chat_id, sql::SQLString word) {
  std::shared_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement(
      "DELETE FROM test.Word WHERE chat_id = ? AND word = ?"));
  stmnt->setInt64(1, chat_id);
  stmnt->setString(2, word);
  stmnt->executeUpdate();
}

bool Database::existsWord(sql::SQLString word) {
  std::shared_ptr<sql::PreparedStatement> stmnt(
      conn->prepareStatement("SELECT EXISTS (SELECT * FROM test.Word "
                             "WHERE word = ?) as word_exists"));
  stmnt->setString(1, word);
  std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
  while (res->next()) {
    return res->getBoolean("word_exists");
  }
  return false;
}

std::vector<Word> Database::getActualWords() {
  std::shared_ptr<sql::Statement> stmnt(conn->createStatement());
  std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery(
      "SELECT * FROM test.Word ORDER BY to_delete DESC LIMIT 1"));
  std::vector<Word> result;
  while (res->next()) {
    result.push_back(Word{
        res->getInt64("chat_id"), res->getString("word").c_str(),
        res->getString("translation").c_str(), res->getShort("to_delete")});
  }
  return result;
}
