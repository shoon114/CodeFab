#ifdef _DEBUG
#include "gmock/gmock.h"
using namespace testing;
#endif

int main() {
#ifdef _DEBUG
	InitGoogleMock();
	return RUN_ALL_TESTS();
#else
	return 0;
#endif
}