#include <gtest/gtest.h>
#include <vector>
#include "../utils/Transform.h"
#include <numeric>

TEST(TransformTestsDynamic, SuccTestSolo) {
    auto func = [](int x) {
        return x + 1;
    };

    std::vector<int> v = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses<DYN>(v, func, 1);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsDynamic, SuccTest) {
    auto func = [](int x) {
        return x + 1;
    };

    std::vector<int> v = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses<DYN>(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsDynamic, DifferentTypeTest) {
    auto func = [](int x) {
        return x % 2 == 0;
    };

    std::vector<int> v = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses<DYN>(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsDynamic, BigTest) {
    auto func = [](int x) {
        return x * x * x + 2 * x * x - x * 4 - 1;
    };

    std::vector<int> v(10000);
    std::iota(v.begin(), v.end(), 0);
    auto ans = TransformWithProcesses<DYN>(v, func, 16);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsStatic, SuccTestSolo) {
    auto func = [](int x) {
        return x + 1;
    };

    std::vector<int> v = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses<STAT>(v, func, 1);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsStatic, SuccTest) {
    auto func = [](int x) {
        return x + 1;
    };

    std::vector<int> v = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses<STAT>(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsStatic, DifferentTypeTest) {
    auto func = [](int x) {
        return x % 2 == 0;
    };

    std::vector<int> v = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };
    auto ans = TransformWithProcesses<STAT>(v, func, 3);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsStatic, BigTest) {
    auto func = [](int x) {
        return x * x * x + 2 * x * x - x * 4 - 1;
    };

    std::vector<int> v(10000);
    std::iota(v.begin(), v.end(), 0);
    auto ans = TransformWithProcesses<STAT>(v, func, 16);
    for (int i = 0; i < v.size(); i++) {
        ASSERT_EQ(ans[i], func(v[i]));
    }
}

TEST(TransformTestsStatic, ErrorTest) {
    auto func = [](int x) {
        throw std::runtime_error("Test error");
        return x;
    };

    std::vector<int> v(100, 1);
    bool failed = false;
    try {
        auto ans = TransformWithProcesses<STAT>(v, func, 16);
    } catch (std::exception &e) {
        failed = true;
    }
    ASSERT_TRUE(failed);
}

TEST(TransformTestsDynamic, ErrorTest) {
    auto func = [](int x) {
        throw std::runtime_error("Test error");
        return x;
    };

    std::vector<int> v(100, 1);
    bool failed = false;
    try {
        auto ans = TransformWithProcesses<DYN>(v, func, 16);
    } catch (std::exception &e) {
        failed = true;
    }
    ASSERT_TRUE(failed);
}

TEST(TransformTestsDynamic, ErrorTestSleep) {
    auto func = [](int x) {
        sleep(x % 3 + 1);
        throw std::runtime_error("Test error");
        return x;
    };

    std::vector<int> v(100);
    std::iota(v.begin(), v.end(), 0);
    bool failed = false;
    try {
        auto ans = TransformWithProcesses<DYN>(v, func, 16);
    } catch (std::exception &e) {
        failed = true;
    }
    ASSERT_TRUE(failed);
}

TEST(TransformTestsStatic, ErrorTestSleep) {
    auto func = [](int x) {
        sleep(x % 3 + 1);
        throw std::runtime_error("Test error");
        return x;
    };

    std::vector<int> v(100);
    std::iota(v.begin(), v.end(), 0);
    bool failed = false;
    try {
        auto ans = TransformWithProcesses<STAT>(v, func, 16);
    } catch (std::exception &e) {
        failed = true;
    }
    ASSERT_TRUE(failed);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}