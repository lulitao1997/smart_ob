#include <gtest/gtest.h>
#include <ob_updater.h>
#include <reader.h>

#include "ob.h"
#include "types.h"

TEST(Test, TestLevel3Ob) {
    auto ob = L3Book{};
    ob.Add(1, true, 2, 123);
}