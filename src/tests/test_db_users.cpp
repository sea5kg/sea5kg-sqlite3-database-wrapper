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
 * Official Source Code: https://github.com/sea5kg/sea5kg-sqlite3-wrapper
 *
 ***********************************************************************************/

#include "sea5kg/sqlite3_wrapper/sea5kg_sqlite3_wrapper.h"
#include <iostream>

CLASS_DATABASE_UPDATE_BEGIN(db_users, initial, v001, "Init table users") {
  // IF NOT EXISTS
  return db->execute_query(
    "CREATE TABLE IF NOT EXISTS users ( "
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  uuid VARCHAR(36) NOT NULL,"
    "  name VARCHAR(128) NOT NULL,"
    "  pass VARCHAR(40) NOT NULL,"
    "  salt VARCHAR(40) NOT NULL,"
    "  role VARCHAR(36) NOT NULL,"
    "  dt INTEGER NOT NULL"
    ");",
    error
  );
}
CLASS_DATABASE_UPDATE_NEXT(db_users, v001, v002, "Init table roles") {
  // IF NOT EXISTS
  return db->execute_query(
    "CREATE TABLE IF NOT EXISTS roles ( "
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  role VARCHAR(36) NOT NULL"
    ");",
    error
  );
}
CLASS_DATABASE_UPDATE_END()

class db_users : public sea5kg::sqlite3_wrapper::database_file {
public:
  db_users(const std::string &db_dir) : sea5kg::sqlite3_wrapper::database_file("db_users", db_dir, "test_db_users.db") {
  }
};

int main() {

  // remove previous database
  remove("test_db_users.db");

  int driver_init_ret;
  if (!sea5kg::sqlite3_wrapper::global::init_driver_sqlite3(driver_init_ret)) {
    std::cerr << "Failed to initialize build-in sqlite3 library: " + std::to_string(driver_init_ret);
    return -1;
  }

  db_users db("./");

  std::string error;
  if (!db.open(error)) {
    std::cerr << "Could not open database: " << error << std::endl;
    return -1;
  }

  if (!db.execute_query("DELETE FROM users;", error)) {
    std::cerr << "Could not delete records from users. Error: " << error << std::endl;
    return -1;
  }

  if (!db.execute_query("DELETE FROM roles;", error)) {
    std::cerr << "Could not delete records from roles. Error: " << error << std::endl;
    return -1;
  }

  // execute request on no existing table
  if (db.execute_query("DELETE FROM roles_some_non_existing_table;", error)) {
    std::cerr << "Why we can delete records on non existsing table. Error: " << error << std::endl;
    return -1;
  } else {
    const std::string expected_error = "Problem with execute sql=query: 'no such table: roles_some_non_existing_table'. Incoming SQL: DELETE FROM roles_some_non_existing_table;";
    if (error != expected_error) {
      std::cerr << "Expected error message: " << expected_error << ", but got: " << error << std::endl;
      return -1;
    }
  }

  int count = db.select_sum_or_count("SELECT COUNT(*) FROM roles;", error);
  if (count != 0) {
    std::cerr << "Expected roles is 0 but got: " << count << ", Error: " << error << std::endl;
    return -1;
  }

  if (!db.execute_query("INSERT INTO roles(role) VALUES('role1');", error)) {
    std::cerr << "Could not insert 'role1' to roles. Error: " << error << std::endl;
    return -1;
  }

  count = db.select_sum_or_count("SELECT COUNT(*) FROM roles;", error);
  if (count != 1) {
    std::cerr << "Expected roles is 1 but got: " << count << ", Error: " << error << std::endl;
    return -1;
  }

  if (!db.execute_query("INSERT INTO roles(role) VALUES('role2');", error)) {
    std::cerr << "Could not insert 'role2' to roles. Error: " << error << std::endl;
    return -1;
  }

  if (!db.execute_query("INSERT INTO roles(role) VALUES('role3');", error)) {
    std::cerr << "Could not insert 'role3' to roles. Error: " << error << std::endl;
    return -1;
  }

  count = db.select_sum_or_count("SELECT COUNT(*) FROM roles;", error);
  if (count != 3) {
    std::cerr << "Expected roles is 3 but got: " << count << ", Error: " << error << std::endl;
    return -1;
  }

  sea5kg::sqlite3_wrapper::rows_iterator it;
  if (!db.select_rows("SELECT id, role FROM roles;", it, error)) {
    std::cerr << "Unexpected error for select, Error: " << error << std::endl;
    return -1;
  }

  std::vector<std::string> expected_roles = {"role1", "role2", "role3"};
  std::vector<std::string> got_roles;
  int i_exp_role = 0;
  while (it.next()) {
    long id = it.as_long(0);
    // std::cout << "id = " << id << std::endl;
    std::string role = it.as_string(1);
    got_roles.push_back(role);
    if (role != expected_roles[i_exp_role]) {
      std::cerr << "Expected role: " << expected_roles[i_exp_role] << ", but got " << role << std::endl;
      return -1;
    }
    i_exp_role++;
  }

  sea5kg::sqlite3_wrapper::global::shutdown_driver_sqlite3();
  return 0;
}