#ifndef DATABASE_H
#define DATABASE_H

#include <mariadb/conncpp.hpp>
#include <unordered_map>

class Database {
    private:
        std::unique_ptr<sql::Connection> conn;
    public:
      Database();

      void addWord(int64_t chat_id, sql::SQLString word, sql::SQLString translation,
                   int to_delete);

      int getWordDeletionCount(int64_t chat_id, sql::SQLString word);

      void incrementWordDeletionCount(int64_t chat_id, sql::SQLString word);

      void deleteWord(int64_t chat_id, sql::SQLString word);

      bool existsWord(sql::SQLString word);

      std::unordered_map<int64_t, std::string> getActualWords();
};

#endif //DATABASE_H
