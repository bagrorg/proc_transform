#include "utils/Transform.h"
#include <iostream>
#include <vector>

struct Point {
    int x, y;
};

bool is_in_sphere(Point p) {
    return p.x * p.x + p.y * p.y <= 1;
}

int main() {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto ans = TransformWithProcesses<DYN>(data, [](int x) {return x * 3 + x * 2;}, 3);
    //std::cout << ans.size() << std::endl;
    for (auto e: ans) {
        std::cout << e << std::endl;
    }
}