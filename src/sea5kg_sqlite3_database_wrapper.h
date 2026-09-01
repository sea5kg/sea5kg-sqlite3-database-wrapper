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

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sea5kg {

class database_update_info {
public:
  database_update_info(const std::string &version_from, const std::string &version_to, const std::string &description);
  const std::string &version_from() const;
  const std::string &version_to() const;
  const std::string &description() const;

private:
  std::string m_version_from;
  std::string m_version_to;
  std::string m_description;
};

class database_file;

class database_update {
public:
  database_update(const std::string &version_from, const std::string &version_to, const std::string &description);
  const database_update_info &info();
  void set_weight(int weight);
  int weight();
  virtual bool apply_update(database_file *pDatabaseFile) = 0;

protected:
  std::string TAG;

private:
  database_update_info m_update_info;
  int m_weight;
};

#define DATABASE_UPDATE_BEGIN(class_name, ver_from, ver_to, description)                                               \
  class db_update_##class_name##_##ver_from##_##ver_to : public sea5kg::database_update {                              \
  public:                                                                                                              \
    db_update_##class_name##_##ver_from##_##ver_to() : sea5kg::database_update(#ver_from, #ver_to, description) {      \
    }                                                                                                                  \
    virtual bool apply_update(sea5kg::database_file *db) override {

#define DATABASE_UPDATE_END()                                                                                          \
  }                                                                                                                    \
  }                                                                                                                    \
  db_update_##class_name##_##ver_from##_##ver_to##_impl;

#define ADD_DATABASE_UPDATE(class_name, ver_from, ver_to)                                                              \
  m_vDbUpdates.push_back(std::make_shared<db_update_##class_name##_##ver_from##_##ver_to>());

class DatabaseSelectRows {
public:
  DatabaseSelectRows();
  ~DatabaseSelectRows();
  void setQuery(void *pQuery);
  bool next();
  std::string getString(int nColumnNumber);
  long getLong(int nColumnNumber);

private:
  // hidden type 'sqlite3_stmt *'
  void *m_pQuery;
};

class database_file {
public:
  database_file(const std::string &db_dir, const std::string &filename, long backup_freq = 0);
  ~database_file();
  const std::string &filename() const;
  const std::string &filepath() const;
  bool open();
  bool executeQuery(std::string sSqlInsert);
  int selectSumOrCount(std::string sSqlSelectCount);
  bool selectRows(std::string sSqlSelectRows, DatabaseSelectRows &selectRows);

protected:
  bool installUpdates();
  bool insertDbVersion(const database_update_info &info);
  std::vector<std::shared_ptr<database_update>> m_vDbUpdates;
  std::string TAG;

private:
  void copy_database_to_backup();
  std::mutex m_mutex;

  // hidden type 'sqlite3 *'
  void *m_pDatabaseFile;
  std::string m_filename;
  std::string m_filepath;
  std::string m_basename_backup_filepath;
  long m_last_backup_time;
  long m_backup_freq_in_seconds;
};

} // namespace sea5kg