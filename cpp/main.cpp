#include "AsyncTqdm.hpp"
#include <iostream>
#include <string>
#include <thread>

int main() {
    std::string fifoPath =
      (std::string) "/tmp/" + getlogin() + "/progress.pipe";
    std::cout << "Writing to " << fifoPath << "...\n";
    AsyncTqdm asyncTqdm(0.01, fifoPath);
    asyncTqdm.init("running", 1000);
    for (int i = 0; i < 1000; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        asyncTqdm.update();
    }
    asyncTqdm.complete("complete");
    return 0;
}
