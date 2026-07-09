#ifdef _DEBUG
#include "gmock/gmock.h"
#include "WatchList.h"
#include "ExecutionObserver.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <memory>
#include <string>

using namespace testing;

TEST(WatchListTest, Add_NewName_IsInList) {
	WatchList watchList;
	watchList.Add("a");

	EXPECT_TRUE(watchList.Contains("a"));
}

TEST(WatchListTest, Remove_WatchedName_IsNoLongerInList) {
	WatchList watchList;
	watchList.Add("a");
	
	watchList.Remove("a");//unWatch

	EXPECT_FALSE(watchList.Contains("a"));
}
#endif
