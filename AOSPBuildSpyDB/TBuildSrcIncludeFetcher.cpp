#include "TBuildSrcIncludeFetcher.h"
#include <regex>

TBuildSrcIncludeFetcher::TBuildSrcIncludeFetcher(TSQLiteTable& make, TSQLiteDB& db, const std::string& name, const std::vector<std::string>& fields, const std::vector<int>& primaryKeys)
	: TSQLiteTable(db, name, fields, primaryKeys), m_make(&make)
{

}

void TBuildSrcIncludeFetcher::RecordTargetModule(std::string module)
{
	Item moduleItem;

	for (const std::string& field : m_make->Fields())
	{
		std::set<std::string> set;
		std::pair<std::string, std::set<std::string>> pair =
			std::make_pair(field, set);
		moduleItem.push_back(pair);
	}

	std::function<bool(const std::vector<std::string>&, Item*, void*)> process(CollectModuleInfo);
	m_make->Select<Item, void>(m_make->Fields(), "ModuleName = '" + module + "'", &process, &moduleItem, NULL);
	

	if (moduleItem[1].first != "ModuleDir" || moduleItem[1].second.size() != 1)
		throw std::runtime_error("ModuleDir Err");

	std::regex re("\\s+");
	/*
	 *  IncludeDirs
	 */
	std::set<std::string> includeDirsSet;

	if (moduleItem[2].first != "ExportIncludeDirs")
		throw std::runtime_error("Index 2 field is not ExportIncludeDirs");
	for (const std::string& dirs : moduleItem[2].second)
	{
		std::sregex_token_iterator it(dirs.begin(), dirs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			if (includeDirsSet.find(*it) == includeDirsSet.end())
				includeDirsSet.insert(*it);
		}
	}

	if (moduleItem[3].first != "IncludeDirs")
		throw std::runtime_error("Index 3 field is not IncludeDirs");
	for (const std::string& dirs : moduleItem[3].second)
	{
		std::sregex_token_iterator it(dirs.begin(), dirs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			if (includeDirsSet.find(*it) == includeDirsSet.end())
				includeDirsSet.insert(*it);
		}
	}

	if (moduleItem[8].first != "HeaderLibs")
		throw std::runtime_error("Index 8 field is not HeaderLibs");
	for (const std::string& dirs : moduleItem[8].second)
	{
		std::sregex_token_iterator it(dirs.begin(), dirs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			GetHeaderLibIncludeDirs(*it, &includeDirsSet);
		}
	}

	std::string includeDirs = "";
	for (const std::string& dir : includeDirsSet)
		includeDirs += dir + " ";
	if (includeDirsSet.size() > 0)
		includeDirs.pop_back();

	/*
	 * Srcs
	 */
	std::string srcs = "";
	for (const std::string& src : moduleItem[4].second)
		srcs += src + " ";
	if (moduleItem[4].second.size() > 0)
		srcs.pop_back();

	/*
	 * Insert
	 */
	Insert({ module, srcs, includeDirs });

	/*
	 * Dep 
	 */
	if (moduleItem[5].first != "SharedLibs")
		throw std::runtime_error("Index 5 field is not SharedLibs");
	for (const std::string& libs : moduleItem[5].second)
	{
		std::sregex_token_iterator it(libs.begin(), libs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			RecordTargetModule(*it);
		}
	}

	if (moduleItem[6].first != "WholeStaticLibs")
		throw std::runtime_error("Index 6 field is not WholeStaticLibs");
	for (const std::string& libs : moduleItem[6].second)
	{
		std::sregex_token_iterator it(libs.begin(), libs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			RecordTargetModule(*it);
		}
	}

	if (moduleItem[7].first != "StaticLibs")
		throw std::runtime_error("Index 7 field is not StaticLibs");
	for (const std::string& libs : moduleItem[7].second)
	{
		std::sregex_token_iterator it(libs.begin(), libs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			RecordTargetModule(*it);
		}
	}
}


bool TBuildSrcIncludeFetcher::CollectModuleInfo(const std::vector<std::string>& res, Item* item, void*)
{
	for (int i = 0; i < res.size(); i++)
	{
		std::set<std::string>& set = (*item)[i].second;
		if (set.find(res[i]) == set.end())
			set.insert(res[i]);
	}

	return true;
}

void TBuildSrcIncludeFetcher::GetHeaderLibIncludeDirs(std::string headerLib, std::set<std::string>* set)
{
	Item moduleItem;

	for (const std::string& field : m_make->Fields())
	{
		std::set<std::string> set;
		std::pair<std::string, std::set<std::string>> pair =
			std::make_pair(field, set);
		moduleItem.push_back(pair);
	}

	std::function<bool(const std::vector<std::string>&, Item*, void*)> process(CollectModuleInfo);
	m_make->Select<Item, void>(m_make->Fields(), "ModuleName = '" + headerLib + "'", &process, &moduleItem, NULL);

	std::regex re("\\s+");

	if (moduleItem[2].first != "ExportIncludeDirs")
		throw std::runtime_error("Index 2 field is not ExportIncludeDirs");
	for (const std::string& dirs : moduleItem[2].second)
	{
		std::sregex_token_iterator it(dirs.begin(), dirs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			if (set->find(*it) == set->end())
				set->insert(*it);
		}
	}

	if (moduleItem[3].first != "IncludeDirs")
		throw std::runtime_error("Index 3 field is not IncludeDirs");
	for (const std::string& dirs : moduleItem[3].second)
	{
		std::sregex_token_iterator it(dirs.begin(), dirs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			if (set->find(*it) == set->end())
				set->insert(*it);
		}
	}

	if (moduleItem[8].first != "HeaderLibs")
		throw std::runtime_error("Index 8 field is not IncludeDirs");
	for (const std::string& dirs : moduleItem[8].second)
	{
		std::sregex_token_iterator it(dirs.begin(), dirs.end(), re, -1);
		std::sregex_token_iterator end;
		for (; it != end; ++it)
		{
			GetHeaderLibIncludeDirs(*it, set);
		}
	}
}