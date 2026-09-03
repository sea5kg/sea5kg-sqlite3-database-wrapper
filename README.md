# sea5kg-sqlite3-wrapper

C++ Class-helper for working with sqlite3 database files

## How to integrate to your project

via wsjcpp:
```
$ wsjcpp install https://github.com/sea5kg/sea5kg-sqlite3-wrapper:master
```

or include this files:

- `src/sea5kg/sqlite3_wrapper/sea5kg_sqlite3_wrapper.h`
- `src/sea5kg/sqlite3_wrapper/sea5kg_sqlite3_wrapper.cpp`

Also you need add sqlite3:

- First way: you can copy from current repository by path `src/third_party/sqlite3/*` and add it to include paths (`src/third_party/sqlite3/src`).
- Second way: you can download from official web-site https://sqlite.org/download.html (I tested with sqlite-amalgamation)

## How use this

Example main func:
```cpp
#include <iostream>
#include "sea5kg/sqlite3_wrapper/sea5kg_sqlite3_wrapper.h"

// prepare updates

CLASS_DATABASE_UPDATE_BEGIN(sea5kg_sqlite3_wrapper, initial, v001, "Init table users") {
  // IF NOT EXISTS
  return db->execute_query(
    "CREATE TABLE IF NOT EXISTS table1 ( "
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  row VARCHAR(36) NOT NULL"
    ");",
    error
  );
}
CLASS_DATABASE_UPDATE_NEXT(sea5kg_sqlite3_wrapper, v001, v002, "Init table roles") {
  // IF NOT EXISTS
  return db->execute_query(
    "INSERT INTO table1(row) VALUES ('85c57f38-a799-11f1-ac9c-afd28f9c0337');",
    error
  );
}
CLASS_DATABASE_UPDATE_END()

int main(int argc, const char *argv[]) {
  sea5kg::sqlite3_wrapper::database_file db("sea5kg_sqlite3_wrapper");
  std::string error;
  if (!db.open(error)) {
    std::cerr << error << std::endl;
    return -1;
  }

  if (!db.contains_table("table1")) {
    std::cerr << "Not contains table: table1" << std::endl;
    return -1;
  }

  db.execute_query("INSERT INTO table1(row) VALUES ('85c57f38-a799-11f1-ac9c-afd28f9c0338');", error);
  db.execute_query("INSERT INTO table1(row) VALUES ('85c57f38-a799-11f1-ac9c-afd28f9c0339');", error);
  db.execute_query("INSERT INTO table1(row) VALUES ('85c57f38-a799-11f1-ac9c-afd28f9c0340');", error);

  sea5kg::sqlite3_wrapper::rows_iterator it;
  db.select_rows("SELECT id, row FROM table1;", it, error);
  while (it.next()) {
    std::cout << it.as_long(0) << ": " << it.as_string(1) << std::endl;
  }

  return 0;
}
```

Example output:

```
1: 85c57f38-a799-11f1-ac9c-afd28f9c0337
2: 85c57f38-a799-11f1-ac9c-afd28f9c0338
3: 85c57f38-a799-11f1-ac9c-afd28f9c0339
4: 85c57f38-a799-11f1-ac9c-afd28f9c0340
```
