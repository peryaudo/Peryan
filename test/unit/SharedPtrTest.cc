#include "gtest/gtest.h"
#include "../../src/SharedPtr.h"

#include <iostream>

namespace {

class Foo {
public:
	int& constructed_;
	int& destructed_;
	Foo(int& constructed, int& destructed)
		: constructed_(constructed), destructed_(destructed) {
		++constructed_;
	}
	Foo(const Foo& foo) : constructed_(foo.constructed_), destructed_(foo.destructed_) {
		++constructed_;
	}
	~Foo() {
		++destructed_;
	}
};

TEST(SharedPtrTest, BasicUse) {
	int constructed = 0, destructed = 0;
	{
		using namespace Peryan;
		SharedPtr<Foo> foo(new Foo(constructed, destructed));
		SharedPtr<Foo> bar = foo;
	}
	ASSERT_TRUE(constructed == destructed);
	ASSERT_EQ(1, constructed);
}

}
