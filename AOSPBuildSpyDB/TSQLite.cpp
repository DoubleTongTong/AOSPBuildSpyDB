#include "TSQLite.h"

TSQLiteDB::TSQLiteDB(const std::string& db_file)
{
    int rc = sqlite3_open(db_file.c_str(), &m_db);
    if (rc != SQLITE_OK) 
    {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(m_db)));
    }
}

TSQLiteDB::~TSQLiteDB() {
    sqlite3_close(m_db);
}

void TSQLiteDB::Exec(const std::string& sql)
{
    char* error_message = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error("Cannot execute SQL: " + std::string(error_message));
    }
}

void TSQLiteDB::BeginTransaction()
{
    Exec("BEGIN TRANSACTION");
}

void TSQLiteDB::RollbackTransaction()
{
    Exec("ROLLBACK");
}

void TSQLiteDB::CommitTransaction()
{
    Exec("COMMIT");
}

void TSQLiteDB::PrepareStatement(const std::string& sql, sqlite3_stmt** stmt)
{
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, stmt, NULL);
    if (rc != SQLITE_OK) 
    {
        std::string errorMsg = sqlite3_errmsg(m_db);
        throw std::runtime_error("Error preparing SQL statement: " + errorMsg);
    }
}

bool TSQLiteDB::TableExists(const std::string& name)
{
    std::string query = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + name + "';";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db)));
    }
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW;
}

void TSQLiteDB::GetTableFields(const std::string& name, std::vector<std::string>* fields, std::vector<int>* primaryKeys)
{
    /*
     * cid         name        type        notnull     dflt_value  pk        
       ----------  ----------  ----------  ----------  ----------  ----------
       0           id          INTEGER     1                       1         
       1           name        TEXT        1                               
       2           age         INTEGER     0                               
       3           email       TEXT        0  
     */
    std::string sql = "PRAGMA table_info(" + name + ");";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(m_db)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string fieldName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        bool isPrimaryKey = sqlite3_column_int(stmt, 5) != 0;
        fields->push_back(fieldName);
        if (isPrimaryKey)
            primaryKeys->push_back(fields->size() - 1);
    }

    sqlite3_finalize(stmt);
}



TSQLiteTable::TSQLiteTable()
{

}

TSQLiteTable::TSQLiteTable(TSQLiteDB& db, const std::string& name, const std::vector<std::string>& fields, const std::vector<int>& primaryKeys)
    : m_db(&db), m_name(name), m_fields(fields), m_primaryKeys(primaryKeys)
{
    if (m_db->TableExists(m_name))
    {
        // 检查表是否相同
        std::vector<std::string> existingFields;
        std::vector<int> existingPrimaryKeys;

        db.GetTableFields(m_name, &existingFields, &existingPrimaryKeys);
        if (existingFields != m_fields || existingPrimaryKeys != m_primaryKeys) {
            throw std::runtime_error("Error in TSQLiteTable::TSQLiteTable: Table " + m_name + " already exists with different schema.");
        }
    }
    else
    {
        // 创建新表
        std::string sql = "CREATE TABLE " + m_name + "(";

        // 拼接字段名和类型
        for (const auto& field : m_fields) {
            sql += field + " TEXT, ";
        }

        // 拼接主键
        if (!m_primaryKeys.empty()) {
            sql += "PRIMARY KEY (";
            for (size_t i = 0; i < m_primaryKeys.size(); ++i) {
                sql += m_fields[m_primaryKeys[i]] + ", ";
            }
            // 去除最后一个逗号和空格
            sql.pop_back();
            sql.pop_back();
            sql += ")";
        }
        else {
            // 去除最后一个逗号和空格
            sql.pop_back();
            sql.pop_back();
        }

        sql += ");";

        // 执行 SQL 语句
        m_db->Exec(sql);
    }
}

void TSQLiteTable::Insert(const std::vector<std::string>& values) {
    // 根据字段向量动态生成插入语句
    std::string sql = "INSERT INTO " + m_name + " VALUES (";
    for (size_t i = 0; i < values.size(); i++) {
        sql += "'" + values[i] + "'";
        if (i < values.size() - 1) {
            sql += ", ";
        }
    }
    sql += ")";
    m_db->Exec(sql);
}

const std::vector<std::string>& TSQLiteTable::Fields() const
{
    return m_fields;
}