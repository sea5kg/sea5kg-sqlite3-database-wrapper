/**********************************************************************************
 * MIT License
 *
 * Copyright (c) 2025-2026 Evgenii Sopov <mrseakg@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Official Source Code: https://github.com/sea5kg/sea5kg-sql-builder
 *
 ***********************************************************************************/

#include <iostream>
#include "sea5kg/sqlite3_wrapper/sea5kg_sqlite3_wrapper.h"

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
