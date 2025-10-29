#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <cryo/set.h>

TEST_CASE("Arena") {
    cryo::Set set(23);
    CHECK(set.key() == 23);
}
