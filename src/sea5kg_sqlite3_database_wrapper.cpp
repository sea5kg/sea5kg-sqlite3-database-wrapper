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

#include "sea5kg_sqlite3_database_wrapper.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace sea5kg {

namespace sqlite3_wrapper {

bool __file_exists(const std::string &sFilename) {
  struct stat st;
  bool bExists = (stat(sFilename.c_str(), &st) == 0);
  if (bExists) {
    return (st.st_mode & S_IFDIR) == 0;
  }
  return false;
}

bool __dir_exists(const std::string &sDirname) {
  struct stat st;
  bool bExists = (stat(sDirname.c_str(), &st) == 0);
  if (bExists) {
    return (st.st_mode & S_IFDIR) != 0;
  }
  return false;
}

bool __make_dir(const std::string &sDirname) {
  struct stat st;

  const std::filesystem::path dir{sDirname};
  std::filesystem::create_directory(dir);

  int nStatus = mkdir(sDirname.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (nStatus == 0) {
    return true;
  }
  if (nStatus == EACCES) {
    std::cout << "FAILED create folder " << sDirname << std::endl;
    return false;
  }
  // std::cout << "nStatus: " << nStatus << std::endl;
  return true;
}

bool __copy_file(const std::string &sSourceFilename, const std::string &sTargetFilename) {
  if (!__file_exists(sSourceFilename)) {
    // WsjcppLog::err("copyFile", "File '" + sSourceFilename + "' did not exists");
    return false;
  }

  if (__file_exists(sTargetFilename)) {
    // WsjcppLog::err("copyFile", "File '" + sTargetFilename + "' already exists");
    return false;
  }

  std::ifstream src(sSourceFilename, std::ios::binary);
  if (!src.is_open()) {
    // WsjcppLog::err("copyFile", "Could not open file '" + sSourceFilename + "' for read");
    return false;
  }

  std::ofstream dst(sTargetFilename, std::ios::binary);
  if (!dst.is_open()) {
    // WsjcppLog::err("copyFile", "Could not open file '" + sTargetFilename + "' for write");
    return false;
  }

  dst << src.rdbuf();
  return true;
}

// std::map<std::string, database_file *> *g_opened_database_files = nullptr;

// // static
// void global_databases::add_opened_database_file(const std::string &name, database_file *db) {
//   if (g_opened_database_files == nullptr) {
//     // sea5kg::log::info(std::string(), "Create employees map");
//     g_opened_database_files = new std::map<std::string, database_file *>();
//   }
//   if (g_opened_database_files->find(name) != g_opened_database_files->end()) {
//     sea5kg::log::critical("WsjcppEmployees::addService", "Already registered '" + name + "'");
//   } else {
//     g_opened_database_files->insert(std::pair<std::string, database_file *>(name, db));
//   }
// }

// // static
// bool global_databases::init_driver_sqlite3(int &ret) {
//   ret = sqlite3_initialize();
//   return SQLITE_OK == ret;
// }

// // static
// void global_databases::shutdown_driver_sqlite3() {
//   // will be automatically closed all opened databases
//   if (g_opened_database_files != nullptr) {
//     for (const auto &pair : *g_opened_database_files) {
//       pair.second->close();
//     }
//   }
//   sqlite3_shutdown();
// }

// ---------------------------------------------------------------------
// database_update_info

database_update_info::database_update_info(
  const std::string &version_from, const std::string &version_to, const std::string &description
)
    : m_version_from(version_from), m_version_to(version_to), m_description(description) {
}

const std::string &database_update_info::version_from() const {
  return m_version_from;
}

const std::string &database_update_info::version_to() const {
  return m_version_to;
}

const std::string &database_update_info::description() const {
  return m_description;
}

// ---------------------------------------------------------------------
// database_update

database_update::database_update(
  const std::string &version_from, const std::string &version_to, const std::string &description
)
    : m_update_info(version_from, version_to, description) {
}

const database_update_info &database_update::info() {
  return m_update_info;
};

void database_update::set_weight(int weight) {
  m_weight = weight;
}

int database_update::weight() {
  return m_weight;
}

// ---------------------------------------------------------------------
// DatabaseSelectRows

DatabaseSelectRows::DatabaseSelectRows() {
  m_pQuery = nullptr;
}

DatabaseSelectRows::~DatabaseSelectRows() {
  if (m_pQuery != nullptr) {
    sqlite3_finalize((sqlite3_stmt *)m_pQuery);
  }
}

void DatabaseSelectRows::setQuery(void *pQuery) {
  m_pQuery = pQuery;
}

bool DatabaseSelectRows::next() {
  return sqlite3_step((sqlite3_stmt *)m_pQuery) == SQLITE_ROW;
}

std::string DatabaseSelectRows::getString(int nColumnNumber) {
  return std::string((const char *)sqlite3_column_text((sqlite3_stmt *)m_pQuery, nColumnNumber));
}

long DatabaseSelectRows::getLong(int nColumnNumber) {
  return sqlite3_column_int64((sqlite3_stmt *)m_pQuery, nColumnNumber);
}

// ---------------------------------------------------------------------
// database_file

database_file::database_file(const std::string &db_dir, const std::string &filename, long backup_freq)
    : m_backup_freq_in_seconds(backup_freq), m_last_backup_time(0) {
  TAG = "database_file-" + filename;
  m_pDatabaseFile = nullptr;
  m_filename = filename;
  std::string sDatabaseDir = db_dir;
  m_filepath = sDatabaseDir + "/" + m_filename;

  if (m_backup_freq_in_seconds > 0) {
    std::string sDatabaseBackupDir = sDatabaseDir + "/backups";
    if (!__dir_exists(sDatabaseBackupDir)) {
      if (!__make_dir(sDatabaseBackupDir)) {
        // TODO
      }
    }
    m_basename_backup_filepath = sDatabaseBackupDir + "/" + m_filename;
  }
};

database_file::~database_file() {
  if (m_pDatabaseFile != nullptr) {
    sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
    sqlite3_close(db);
  }
}

const std::string &database_file::filename() const {
  return m_filename;
}

const std::string &database_file::filepath() const {
  return m_filepath;
}

bool database_file::open() {
  // TODO if could not open but has backup try open backup
  // open connection to a DB
  sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
  int nRet = sqlite3_open_v2(m_filepath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (nRet != SQLITE_OK) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed to open conn: " + std::to_string(nRet));
    return false;
  }
  m_pDatabaseFile = db;

  const std::string sSqlCheckVersionTable =
    "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='db_version';";

  int cnt = selectSumOrCount(sSqlCheckVersionTable.c_str());
  if (cnt == 0) {
    // create db_version
    const std::string sSqlCreateDbVersion = "CREATE TABLE IF NOT EXISTS db_version ( "
                                            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                            "  version_from VARCHAR(64),"
                                            "  version_to VARCHAR(64),"
                                            "  dt INTEGER NOT NULL,"
                                            "  description VARCHAR(2048) NOT NULL"
                                            ");";
    char *zErrMsg = 0;
    nRet = sqlite3_exec(db, sSqlCreateDbVersion.c_str(), 0, 0, &zErrMsg);
    if (nRet != SQLITE_OK) {
      // TODO
      // WsjcppLog::throw_err(TAG, "Problem with create table: " + std::string(zErrMsg));
      return false;
    }
    // TODO
    // WsjcppLog::info(TAG, "Created table db_version in " + m_sFileFullpath);
  }

  if (!installUpdates()) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Problem with install updates");
    return false;
  }

  // TODO
  // WsjcppLog::ok(TAG, "Opened database file " + m_sFileFullpath);
  copy_database_to_backup();
  return true;
}

bool database_file::executeQuery(std::string sSqlInsert) {
  copy_database_to_backup();
  char *zErrMsg = 0;
  sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
  int nRet = sqlite3_exec(db, sSqlInsert.c_str(), 0, 0, &zErrMsg);
  if (nRet != SQLITE_OK) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Problem with insert: " + std::string(zErrMsg) + "\n SQL-query: " + sSqlInsert);
    return false;
  }
  return true;
}

int database_file::selectSumOrCount(std::string sSqlSelectCount) {
  // copy_database_to_backup();
  sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
  sqlite3_stmt *pQuery = nullptr;
  int ret = sqlite3_prepare_v2(db, sSqlSelectCount.c_str(), -1, &pQuery, NULL);
  // prepare the statement
  if (ret != SQLITE_OK) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed to prepare select count: " + std::string(sqlite3_errmsg(db)) +
    //                               "\n SQL-query: " + sSqlSelectCount);
    return -1;
  }
  // step to 1st row of data
  ret = sqlite3_step(pQuery);
  if (ret != SQLITE_ROW) { // see documentation, this can return more values as success
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed to step for select count or sum: " + std::string(sqlite3_errmsg(db)) +
    //                               "\n SQL-query: " + sSqlSelectCount);
    return -1;
  }
  int nRet = sqlite3_column_int(pQuery, 0);
  if (pQuery != nullptr)
    sqlite3_finalize(pQuery);
  return nRet;
}

bool database_file::selectRows(std::string sSqlSelectRows, DatabaseSelectRows &selectRows) {
  // copy_database_to_backup();
  sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
  sqlite3_stmt *pQuery = nullptr;
  int nRet = sqlite3_prepare_v2(db, sSqlSelectRows.c_str(), -1, &pQuery, NULL);
  // prepare the statement
  if (nRet != SQLITE_OK) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed to prepare select rows: " + std::string(sqlite3_errmsg(db)) +
    //                               "\n SQL-query: " + sSqlSelectRows);
    return false;
  }
  selectRows.setQuery((void *)pQuery);
  return true;
}

bool database_file::installUpdates() {
  sqlite3 *db = (sqlite3 *)m_pDatabaseFile;

  // Installed updates
  std::vector<database_update_info> installedUpdates;
  {
    sqlite3_stmt *pQuery = nullptr;
    const std::string sSqlCurrentVersion =
      "SELECT version_from, version_to, description FROM db_version ORDER BY rowid";
    int ret = sqlite3_prepare_v2(db, sSqlCurrentVersion.c_str(), -1, &pQuery, NULL);
    if (ret != SQLITE_OK) {
      // TODO
      // WsjcppLog::throw_err(TAG, "Failed to prepare: " + std::string(sqlite3_errmsg(db)) +
      //                              "\n SQL-query: " + sSqlCurrentVersion);
      return false;
    }
    ret = sqlite3_step(pQuery);
    while (ret == SQLITE_ROW) {
      database_update_info info(
        std::string((const char *)sqlite3_column_text(pQuery, 0)), // from
        std::string((const char *)sqlite3_column_text(pQuery, 1)), // to
        std::string((const char *)sqlite3_column_text(pQuery, 2))  // decr
      );
      installedUpdates.push_back(info);
      ret = sqlite3_step(pQuery);
    }
    if (pQuery != nullptr)
      sqlite3_finalize(pQuery);
  }

  std::vector<std::string> installedVersionsTo;
  installedVersionsTo.push_back(m_initial_version);
  for (int i = 0; i < installedUpdates.size(); i++) {
    installedVersionsTo.push_back(installedUpdates[i].version_to());
  }

  // install updates
  bool bInstalledNewUpdates = true;
  while (bInstalledNewUpdates) {
    bInstalledNewUpdates = false;
    std::vector<std::string> installedNewUpdates;

    for (const auto &upd : m_vDbUpdates) {
      // database_update *pUpdate = m_vDbUpdates[i];
      const std::string &sVersionFrom = upd->info().version_from();
      const std::string &sVersionTo = upd->info().version_to();

      for (int iv = 0; iv < installedVersionsTo.size(); iv++) {
        if (sVersionFrom == installedVersionsTo[iv]) {
          auto it = std::find(installedVersionsTo.begin(), installedVersionsTo.end(), sVersionTo);
          if (it == installedVersionsTo.end()) {
            if (!upd->apply_update(this)) {
              return false;
            }
            if (!insertDbVersion(upd->info())) {
              return false;
            }
            // TODO
            // WsjcppLog::ok(TAG, "Installed update " + sVersionTo);
            installedNewUpdates.push_back(sVersionTo);
          } else {
            // skip update
          }
        }
      }
    }
    bInstalledNewUpdates = installedNewUpdates.size() > 0;
    for (int i = 0; i < installedNewUpdates.size(); i++) {
      installedVersionsTo.push_back(installedNewUpdates[i]);
    }
  }

  return true;
}

bool database_file::insertDbVersion(const database_update_info &info) {
  // TODO escaping
  long nCurrentTime =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  std::string sSqlDbVersion = "INSERT INTO "
                              "db_version(version_from, version_to, description, dt) "
                              "VALUES(\"" +
                              info.version_from() + "\", \"" + info.version_to() + "\", \"" + info.description() +
                              "\", " + std::to_string(nCurrentTime) + ")";
  return this->executeQuery(sSqlDbVersion);
}

void database_file::copy_database_to_backup() {
  if (m_backup_freq_in_seconds <= 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  // TODO must be configurable
  // every 1 minutes make backup
  long nCurrentTime =
    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  if (nCurrentTime - m_last_backup_time < m_backup_freq_in_seconds) {
    return;
  }
  m_last_backup_time = nCurrentTime;

  int nMaxBackupsFiles = 9;
  // TODO
  // WsjcppLog::info(TAG, "Start backup for " + m_sFileFullpath);
  std::string sFilebackup = m_basename_backup_filepath + "." + std::to_string(nMaxBackupsFiles);
  if (__file_exists(sFilebackup)) {
    if (remove(sFilebackup.c_str())) {
      // TODO error or warning
    }
  }
  for (int i = nMaxBackupsFiles - 1; i >= 0; i--) {
    std::string sFilebackupFrom = m_basename_backup_filepath + "." + std::to_string(i);
    std::string sFilebackupTo = m_basename_backup_filepath + "." + std::to_string(i + 1);
    if (__file_exists(sFilebackupFrom)) {
      if (std::rename(sFilebackupFrom.c_str(), sFilebackupTo.c_str())) {
        // TODO
        // WsjcppLog::throw_err(TAG, "Could not rename from " + sFilebackupFrom + " to " + sFilebackupTo);
      }
    }
  }
  sFilebackup = m_basename_backup_filepath + "." + std::to_string(0);
  if (!__copy_file(m_filepath, sFilebackup)) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed copy file to backup for " + m_sFileFullpath);
  }
  // TODO
  // WsjcppLog::info(TAG, "Backup done for " + m_sFileFullpath);
}

} // namespace sqlite3_wrapper

} // namespace sea5kg
