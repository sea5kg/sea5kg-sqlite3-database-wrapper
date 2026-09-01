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
 * Official Source Code: https://github.com/sea5kg/sea5kg-sqlite3-database-wrapper
 *
 ***********************************************************************************/

#include <iostream>
#include <sea5kg_sqlite3_database_wrapper.h>

DATABASE_UPDATE_BEGIN(test_database_file, v000, v001, "Init table uuids")
// IF NOT EXISTS
return db->executeQuery("CREATE TABLE users ( "
                        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "  uuid VARCHAR(36) NOT NULL,"
                        "  name VARCHAR(128) NOT NULL,"
                        "  pass VARCHAR(40) NOT NULL,"
                        "  salt VARCHAR(40) NOT NULL,"
                        "  role VARCHAR(36) NOT NULL,"
                        "  dt INTEGER NOT NULL"
                        ");");
DATABASE_UPDATE_END()

class test_database_file : public sea5kg::database_file {
public:
  test_database_file() : sea5kg::database_file("./", "test_database_file.db") {
    ADD_DATABASE_UPDATE(test_database_file, v000, v001)
  }
};

int main() {
  test_database_file db;

  if (!db.open()) {
    std::cerr << "Could not open database" << std::endl;
    return -1;
  }

  return 0;
}