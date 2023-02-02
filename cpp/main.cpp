#include <iostream>
#include <string>
#include "AsyncTqdm.hpp"

#include <thread>
int main() {
    std::cout << "Running..." << std::endl;
    AsyncTqdm asyncTqdm(0.01);
    asyncTqdm.init("running", 1000);
    for (int i = 0; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        asyncTqdm.update();
    }
    asyncTqdm.complete("complete");
    return 0;
}
