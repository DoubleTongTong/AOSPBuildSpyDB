#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <utility>
#include <set>
#include <cstdio>
#include <sstream>

#include "TSQLite.h"

template<int... TKeyIndexs>
class TBuildLogParser
{
public:
	TBuildLogParser(const std::string name, const std::string fileName);

	~TBuildLogParser();

	const TSQLiteTable* CreateSQLiteTable(TSQLiteDB& db);

private:
	void Parse(const std::string& fileName);

private:
	const std::string m_name;

	class Comparator
	{
	public:
		bool operator()(const Item& a, const Item& b) const;
	};

	std::set<Item, Comparator> m_set;
	TSQLiteTable m_table;
};



template<int... TKeyIndexs>
TBuildLogParser<TKeyIndexs...>::TBuildLogParser(const std::string name, const std::string fileName)
	: m_name(name)
{
	Parse(fileName);
}

template<int... TKeyIndexs>
TBuildLogParser<TKeyIndexs...>::~TBuildLogParser()
{

}

template<int... TKeyIndexs>
void TBuildLogParser<TKeyIndexs...>::Parse(const std::string& fileName)
{
	std::ifstream infile(fileName);
	if (!infile.is_open()) {
		throw std::runtime_error("Failed to open file: " + fileName);
	}

	/*
	 * TCodeAnalysis ModuleName=libcpuinfo;ModuleDir=external/cpuinfo;Srcs=[src/init.c];ExportIncludeDirs=[include];HeaderLibs=[]
	 */
	 // 定义正则表达式
	std::regex pattern("([A-Za-z]+)=(.*?)(?=;|$)");
	std::regex vecPattern(R"((?:^\[|\s+|^)([^\s\]]*)(?:\s+|\]|$))");

	int lineNum = 1;
	std::string line;
	while (std::getline(infile, line))
	{
		/*
		 * 处理的有点慢，打印处理进度
		 */
		std::printf("\rLine %d is in processing...", lineNum++);

		Item item;

		std::sregex_iterator begin(line.begin(), line.end(), pattern);
		std::sregex_iterator end;
		for (std::sregex_iterator i = begin; i != end; ++i)
		{
			std::set<std::string> set;

			std::smatch match = *i;

			std::string key = match[1].str();
			std::string val = match[2].str();

			std::sregex_iterator vecBegin(val.begin(), val.end(), vecPattern);
			for (std::sregex_iterator j = vecBegin; j != end; ++j)
			{
				std::smatch setMatch = *j;
				std::string e = setMatch[1].str();

				if (e.length() > 0)
					set.insert(e);
			}

			std::pair<std::string, std::set<std::string>> pair =
				std::make_pair(std::move(key), std::move(set));

			item.push_back(std::move(pair));
		}

		// Note
		if (item[9].first != "MakeType" && item[1].first != "ModuleDir")
			throw std::runtime_error("Index 9 field is not MakeType");
		if (*item[9].second.begin() == "bp")
		{
			if (*item[1].second.begin() == ".")
				continue;

			std::string prefix = "defaults";
			if ((*(item[1].second.begin())).substr(0, prefix.length()) == prefix)
				continue;
			prefix = "deps";
			if ((*(item[1].second.begin())).substr(0, prefix.length()) == prefix)
				continue;
		}

		auto it = m_set.find(item);
		if (it != m_set.end())
		{
			//将 item 中没有的元素加入其中
			for (int i = 0; i < item.size(); i++)
			{
				for (const std::string& s : item[i].second)
				{
					std::set<std::string>* pSet = const_cast<std::set<std::string>*>(&((*it)[i].second));
					if (pSet->find(s) == pSet->end())
					{
						pSet->insert(s);
					}
				}
			}
		}
		else
		{
			m_set.insert(std::move(item));
		}
	}
}


template<int... TKeyIndexs>
bool TBuildLogParser<TKeyIndexs...>::Comparator::operator()(const Item& a, const Item& b) const
{
	int index[] = { TKeyIndexs... };
	for (int i = 0; i < sizeof...(TKeyIndexs); ++i) {
		if (*a[index[i]].second.begin() != *b[index[i]].second.begin())
			return *a[index[i]].second.begin() < *b[index[i]].second.begin();
	}

	return false;
}

template<int... TKeyIndexs>
const TSQLiteTable* TBuildLogParser<TKeyIndexs...>::CreateSQLiteTable(TSQLiteDB& db)
{
	std::vector<std::string> fields;
	const Item& item = *m_set.begin();
	for (auto pair : item)
		fields.push_back(pair.first);

	std::vector<int> primaryKeys = { TKeyIndexs... };

	m_table = TSQLiteTable(db, m_name, fields, primaryKeys);

	m_table.BatchInsert(m_set);
	return &m_table;
}