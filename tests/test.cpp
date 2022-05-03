#include <gtest/gtest.h>
#include <vector>
#include "../utils/Transform.h"
#include <numeric>

TEST(TransformTests, SuccTestSolo) {
    auto func = [](int x) {
        return x + 1 == 0;
    };

    std::vector<int> v = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses(v, func, 1);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTests, SuccTest) {
    auto func = [](int x) {
        return x + 1 == 0;
    };

    std::vector<int> v = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTests, DifferentTypeTest) {
    auto func = [](int x) {
        return x % 2 == 0;
    };

    std::vector<int> v = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTests, BigTest) {
    auto func = [](int x) {
        return x * x * x + 2 * x * x - x * 4 - 1;
    };

    std::vector<int> v(10000);
    std::iota(v.begin(), v.end(), 0);
    auto ans = TransformWithProcesses(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}