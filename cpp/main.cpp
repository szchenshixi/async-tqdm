#include "AsyncTqdm.hpp"
#include <iostream>
#include <string>
#include <thread>

int main() {
    constexpr int TOTAL = 500;
    constexpr int SLOW = 20;

    std::string fifoPath =
      (std::string) "/tmp/" + getlogin() + "/progress.pipe";
    std::cout << "Writing to " << fifoPath << "...\n";
    AsyncTqdm asyncTqdm(0.1, fifoPath);
    asyncTqdm.init("DummyTaskName", TOTAL);  // Step1: initialize a task bar
    for (int i = 0; i < SLOW/4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(8000));
        asyncTqdm.update(4);  // Step 2: update the progress by 1
    }
    for (int i = 0; i < TOTAL - SLOW; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        asyncTqdm.update();  // Step 2: update the progress by 1
    }
    asyncTqdm.complete();  // Step 3: set to 100% progress and mark it complete
    return 0;
}
