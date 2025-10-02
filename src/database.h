// database.h
#ifndef database_h
#define database_h

#include <mariadb/conncpp.hpp>
#include <unordered_map>

using namespace std;
using namespace sql;

class Database {
    private:
        unique_ptr<Connection> conn;
    public:
      Database();
      void addWord(int64_t chat_id, SQLString word, SQLString translation,
                   int to_delete);

      int getWordDeletionCount(int64_t chat_id, SQLString word);

      void incrementWordDeletionCount(int64_t chat_id, SQLString word);

      void deleteWord(int64_t chat_id, SQLString word);

      bool existsWord(SQLString word);

      unordered_map<int64_t, string> getActualWords();
};
#endif //database.h
