#pragma once
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <functional>

using Item = std::vector<std::pair<std::string, std::set<std::string>>>;

class TSQLiteDB {
public:
    TSQLiteDB(const std::string& db_file);

    ~TSQLiteDB();

    void Exec(const std::string& sql);
    void BeginTransaction();
    void RollbackTransaction();
    void CommitTransaction();

    void PrepareStatement(const std::string& sql, sqlite3_stmt** stmt);

    bool TableExists(const std::string& name);
    void GetTableFields(const std::string& name, std::vector<std::string>* fields, std::vector<int>* primaryKeys);

private:
    sqlite3* m_db;
};

class TSQLiteTable {
public:
    TSQLiteTable(TSQLiteDB& db, const std::string& name, const std::vector<std::string>& fields, const std::vector<int>& primaryKeys);
    TSQLiteTable();

    void Insert(const std::vector<std::string>& values);

    template<typename Input, typename Output>
    void Select(
        const std::vector<std::string>& fields,
        const std::string& where,
        const std::function<bool(const std::vector<std::string>&, Input*, Output*)>* process,
        Input* input,
        Output* output);

    template<typename T>
    bool BatchInsert(const T& items);

    const std::vector<std::string>& Fields() const;
private:
    TSQLiteDB* m_db;
    std::string m_name;
    std::vector<std::string> m_fields;
    std::vector<int> m_primaryKeys;
};

template<typename T>
bool TSQLiteTable::BatchInsert(const T& items)
{
    m_db->BeginTransaction(); // 开始事务
    for (const auto& item : items) {
        std::string sql = "INSERT INTO " + m_name + " (";
        for (const auto& field : m_fields) {
            sql += field + ",";
        }
        sql.pop_back(); // 删除最后一个逗号

        sql += ") VALUES (";

		for (auto const& pair : item)
		{
			std::string valueStr = "\"";
			for (const std::string& s : pair.second)
				valueStr += s + " ";
            if (pair.second.size() > 0)
                valueStr.pop_back();
			valueStr += "\"";

			sql += valueStr + ",";
		}

        sql.pop_back(); // 删除最后一个逗号
        sql += ");";

        m_db->Exec(sql);
        /*
        int rc = sqlite3_exec(m_db->m_db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            sqlite3_exec(m_db->m_db, "ROLLBACK", nullptr, nullptr, nullptr); // 回滚事务
            return false;
        }
        */
    }
    m_db->CommitTransaction(); // 提交事务
    return true;
}

template<typename Input, typename Output>
void TSQLiteTable::Select(
    const std::vector<std::string>& fields,
    const std::string& where,
    const std::function<bool(const std::vector<std::string>&, Input*, Output*)>* process,
    Input* input,
    Output* output)
{
    std::string sql = "SELECT ";
    if (fields.empty())
        sql += "* ";
    else
    {
        for (const std::string& field : fields)
        {
            if (std::find(m_fields.begin(), m_fields.end(), field) == m_fields.end())
                throw std::runtime_error("Failed to get fields");
            sql += field + ", ";
        }
        sql.pop_back();
        sql.pop_back();
    }
    sql += " FROM " + m_name;

    if (where.empty() == false)
        sql += " WHERE " + where;

    sql += ";";

    sqlite3_stmt* stmt;
    m_db->PrepareStatement(sql.c_str(), &stmt);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::vector<std::string> row;

        for (int i = 0; i < sqlite3_column_count(stmt); ++i)
        {
            const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row.push_back(value ? value : "");
        }

        if (process)
            (*process)(row, input, output);
    }

    sqlite3_finalize(stmt);
}