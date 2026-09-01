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

// ---------------------------------------------------------------------
// database_update_info

database_update_info::database_update_info(const std::string &sVersionFrom, const std::string &sVersionTo,
                                               const std::string &sDescription)
    : m_sVersionFrom(sVersionFrom), m_sVersionTo(sVersionTo), m_sDescription(sDescription) {
}

const std::string &database_update_info::versionFrom() const {
  return m_sVersionFrom;
}

const std::string &database_update_info::versionTo() const {
  return m_sVersionTo;
}

const std::string &database_update_info::description() const {
  return m_sDescription;
}

// ---------------------------------------------------------------------
// database_update

database_update::database_update(const std::string &sVersionFrom, const std::string &sVersionTo,
                                       const std::string &sDescription)
    : m_updateInfo(sVersionFrom, sVersionTo, sDescription) {
}

const database_update_info &database_update::info() {
  return m_updateInfo;
};

void database_update::setWeight(int nWeight) {
  m_nWeight = nWeight;
}

int database_update::getWeight() {
  return m_nWeight;
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

database_file::database_file(const std::string &db_dir, const std::string &filename) {
  TAG = "database_file-" + filename;
  m_pDatabaseFile = nullptr;
  m_sFilename = filename;
  m_nLastBackupTime = 0;
  // EmployConfig *pConfig = findWsjcppEmploy<EmployConfig>();
  std::string sDatabaseDir = db_dir;
  m_sFileFullpath = sDatabaseDir + "/" + m_sFilename;

  std::string sDatabaseBackupDir = sDatabaseDir + "/backups";
  if (!__dir_exists(sDatabaseBackupDir)) {
    if (!__make_dir(sDatabaseBackupDir)) {
      // TODO
    }
  }
  m_sBaseFileBackupFullpath = sDatabaseBackupDir + "/" + m_sFilename;
};

database_file::~database_file() {
  if (m_pDatabaseFile != nullptr) {
    sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
    sqlite3_close(db);
  }
}

std::string database_file::getFilename() {
  return m_sFilename;
}

std::string database_file::getFileFullpath() {
  return m_sFileFullpath;
}

bool database_file::open() {
  // TODO if could not open but has backup try open backup
  // open connection to a DB
  sqlite3 *db = (sqlite3 *)m_pDatabaseFile;
  int nRet = sqlite3_open_v2(m_sFileFullpath.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (nRet != SQLITE_OK) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed to open conn: " + std::to_string(nRet));
    return false;
  }
  m_pDatabaseFile = db;

  const std::string sSqlCheckVersionTable =
      "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='db_version';";

  int nCount = selectSumOrCount(sSqlCheckVersionTable.c_str());
  if (nCount == 0) {
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
  copyDatabaseToBackup();
  return true;
}

bool database_file::executeQuery(std::string sSqlInsert) {
  copyDatabaseToBackup();
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
  // copyDatabaseToBackup();
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
  // copyDatabaseToBackup();
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
      database_update_info info(std::string((const char *)sqlite3_column_text(pQuery, 0)), // from
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
  installedVersionsTo.push_back(""); // start version
  for (int i = 0; i < installedUpdates.size(); i++) {
    installedVersionsTo.push_back(installedUpdates[i].versionTo());
  }

  // install updates
  bool bInstalledNewUpdates = true;
  while (bInstalledNewUpdates) {
    bInstalledNewUpdates = false;
    std::vector<std::string> installedNewUpdates;

    for (const auto &upd : m_vDbUpdates) {
      // database_update *pUpdate = m_vDbUpdates[i];
      const std::string &sVersionFrom = upd->info().versionFrom();
      const std::string &sVersionTo = upd->info().versionTo();

      for (int iv = 0; iv < installedVersionsTo.size(); iv++) {
        if (sVersionFrom == installedVersionsTo[iv]) {
          if (std::find(installedVersionsTo.begin(), installedVersionsTo.end(), sVersionTo) ==
              installedVersionsTo.end()) {
            if (!upd->applyUpdate(this)) {
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
  long nCurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  std::string sSqlDbVersion = "INSERT INTO db_version(version_from, version_to, description, dt) VALUES(\"" +
                              info.versionFrom() + "\", \"" + info.versionTo() + "\", \"" + info.description() +
                              "\", " + std::to_string(nCurrentTime) + ")";
  return this->executeQuery(sSqlDbVersion);
}

void database_file::copyDatabaseToBackup() {
  std::lock_guard<std::mutex> lock(m_mutex);
  // every 1 minutes make backup
  long nCurrentTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  if (nCurrentTime - m_nLastBackupTime < 60) {
    return;
  }
  m_nLastBackupTime = nCurrentTime;

  int nMaxBackupsFiles = 9;
  // TODO
  // WsjcppLog::info(TAG, "Start backup for " + m_sFileFullpath);
  std::string sFilebackup = m_sBaseFileBackupFullpath + "." + std::to_string(nMaxBackupsFiles);
  if (__file_exists(sFilebackup)) {
    if (remove(sFilebackup.c_str())) {
      // TODO error or warning
    }
  }
  for (int i = nMaxBackupsFiles - 1; i >= 0; i--) {
    std::string sFilebackupFrom = m_sBaseFileBackupFullpath + "." + std::to_string(i);
    std::string sFilebackupTo = m_sBaseFileBackupFullpath + "." + std::to_string(i + 1);
    if (__file_exists(sFilebackupFrom)) {
      if (std::rename(sFilebackupFrom.c_str(), sFilebackupTo.c_str())) {
        // TODO
        // WsjcppLog::throw_err(TAG, "Could not rename from " + sFilebackupFrom + " to " + sFilebackupTo);
      }
    }
  }
  sFilebackup = m_sBaseFileBackupFullpath + "." + std::to_string(0);
  if (!__copy_file(m_sFileFullpath, sFilebackup)) {
    // TODO
    // WsjcppLog::throw_err(TAG, "Failed copy file to backup for " + m_sFileFullpath);
  }
  // TODO
  // WsjcppLog::info(TAG, "Backup done for " + m_sFileFullpath);
}

} // namespace sea5kg
