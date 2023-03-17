#include "TSQLite.h"

class TBuildSrcIncludeFetcher : public TSQLiteTable
{
public:
	TBuildSrcIncludeFetcher(TSQLiteTable& make, TSQLiteDB& db, const std::string& name, const std::vector<std::string>& fields, const std::vector<int>& primaryKeys);

	void RecordTargetModule(std::string module);

private:
	TSQLiteTable* m_make;

	class Comparator
	{
	public:
		bool operator()(const Item& a, const Item& b) const;
	};

	std::set<Item, Comparator> m_set;

	static bool CollectModuleInfo(const std::vector<std::string>& res, Item* item, void*);

	void GetHeaderLibIncludeDirs(std::string headerLib, std::set<std::string>* set);
};