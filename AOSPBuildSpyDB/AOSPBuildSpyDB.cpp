// AOSPBuildSpyDB.cpp: 定义应用程序的入口点。
//

#include "AOSPBuildSpyDB.h"
#include "TSQLite.h"
#include "TBuildLogParser.h"
#include "TBuildSrcIncludeFetcher.h"

int main()
{
	TSQLiteDB db("test.db");
	if (db.TableExists("make") == false)
	{
		TBuildLogParser<0, 1> parser("make", R"(D:\AOSPBuildSpyDB\out\build\x64-Debug\AOSPBuildSpyDB\build_log.txt)");
		parser.CreateSQLiteTable(db);
	}
	
	std::vector<std::string> makeFields;
	std::vector<int> makePrimaryKeys;
	db.GetTableFields("make", &makeFields, &makePrimaryKeys);
	
	TSQLiteTable make(db, "make", makeFields, makePrimaryKeys);

	//TBuildSrcIncludeFetcher fetcher(make, db, "build", { "ModuleName", "Srcs", "IncludeDirs" }, {0});
	//fetcher.RecordTargetModule("camera.qcom");
	return 0;
}
