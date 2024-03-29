#ifndef ATQDM_H_
#define ATQDM_H_
#include <array>
#include <assert.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <poll.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>

// File-related variables' naming style I propose:
// For /a/b/c/some.txt
// someDir: /a/b/c
// someFile: file handles or descriptor
// somePath: /a/b/c/some.txt (most path libraries can directly manipulate)
// someFileName: some.txt

// Implementation of this class is based on the POSIX specification on pipe
// "Atomicity of writes less than PIPE_BUF applies to pipes and FIFOs"
class AsyncTqdm {
  public:
    AsyncTqdm(float minInterval = 0.1,
              std::string fifoPath = (std::string) "/tmp/" + getlogin() +
                                     (std::string) "/progress.pipe")
        : mkPid(getpid())
        , mkMinInterval(minInterval) // Seconds to refresh
        , mTotalUpdateAmount(0)
        , mNextUpdateAmount(0)
        , mkFifoPath(std::move(fifoPath))
        , mLastUpdate(std::chrono::high_resolution_clock::now())
        , mHasCompleted(false)
        , mHasInitialized(false)
        , mTotal(0)
        , mRunning(true) {
        if (isNamedPipeListenedTo(mkFifoPath.c_str())) {
            // Named pipe is being listened to
            mFifoFile.open(mkFifoPath, std::fstream::out);
            std::thread watchDog(&AsyncTqdm::watchDogThread, this);
            watchDog.detach();
            std::cout << "Progress bar enabled\n";
        } else {
            std::cout << mkFifoPath << " is not being listened to \n";
            std::cout << "Progress bar disabled\n";
        }
    }

    ~AsyncTqdm() {
        terminate();
        mRunning.store(false, std::memory_order_relaxed);
    }

    // I do not want to over complicate myself so only inplace use is allowed
    AsyncTqdm(const AsyncTqdm&) = delete;
    AsyncTqdm(AsyncTqdm&&) = delete;
    AsyncTqdm& operator=(const AsyncTqdm&) = delete;
    AsyncTqdm& operator=(AsyncTqdm&&) = delete;

    // Ref: https://pubs.opengroup.org/onlinepubs/009696799/functions/poll.html
    static bool isNamedPipeListenedTo(const char* namedPipePath) {
        struct pollfd pfd;
        pfd.fd = open(namedPipePath, O_WRONLY | O_NONBLOCK);
        // Normal data may be written without blocking
        pfd.events = POLLOUT;
        int rv = poll(&pfd, 1, 0);
        close(pfd.fd);
        return (rv > 0);
    }

    int main() {
        const char* namedPipePath = "/path/to/named_pipe";
        if (isNamedPipeListenedTo(namedPipePath)) {
            std::cout << "Named pipe is being listened to" << std::endl;
        } else {
            std::cout << "Named pipe is not being listened to" << std::endl;
        }
        return 0;
    }

    void init(const std::string& desc, int64_t total) {
        if (!mFifoFile.is_open() || mHasInitialized) {
            return;
        }
        if (total < 0) {
            std::cout << "Overflow happened.\n";
            exit(-1);
        }
        mTotal = total;
        std::string toSend = constructInitCmd(mkPid, desc, total);
        // "Atomicity of writes less than PIPE_BUF applies to pipes and FIFOs"
        mFifoFile.write(toSend.c_str(), (long)toSend.length());
        mFifoFile.flush();
        // Initialization guard
        mHasInitialized = true;
    }

    void watchDogThread() {
        while (mRunning.load(std::memory_order_relaxed)) {
            tryRefresh();
            sleep((uint)mkMinInterval);
        }
    }

    void tryRefresh() {
        // There is no shared variable in this function. Thread safe
        if (!mFifoFile.is_open() || !mHasInitialized || mHasCompleted) {
            return;
        }
        auto now = std::chrono::high_resolution_clock::now();
        static decltype(now) lastRefresh = now;  // assign only once
        auto interval = now - lastRefresh;
        constexpr int SEC_TO_NANOSEC = 1e9;
        constexpr int ZERO_UPDATE = 0;
        constexpr int UPDATE_RATE = 1.008;  // 1 time per second

        // Debug
        static int counter = 0;
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(interval)
              .count() > long(SEC_TO_NANOSEC / UPDATE_RATE)) {
            // Debug
            counter = (counter + 1) % 100;
            std::cout << "Watchdog update " << counter << std::endl;
            // Zero update to force refresh on the progress bar
            std::string toSend = constructUpdateCmd(mkPid, ZERO_UPDATE);
            mFifoFile.write(toSend.c_str(), (long)toSend.length());
            mFifoFile.flush();
            lastRefresh = now;
        }
    }

    void update(int64_t undateAmount = 1, bool immediately = false) {
        if (!mFifoFile.is_open() || mHasCompleted) {
            return;
        }
        auto now = std::chrono::high_resolution_clock::now();
        auto interval = now - mLastUpdate;
        mTotalUpdateAmount += undateAmount;
        mNextUpdateAmount += undateAmount;

        constexpr int SEC_TO_NANOSEC = 1e9;
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(interval)
                .count() > long(mkMinInterval * SEC_TO_NANOSEC) ||
            immediately) {
            std::string toSend = constructUpdateCmd(mkPid, mNextUpdateAmount);
            mFifoFile.write(toSend.c_str(), (long)toSend.length());
            mFifoFile.flush();
            mLastUpdate = now;
            mNextUpdateAmount = 0;
        }
    }

    /**
     * @brief Update the progress to 100% and change the status to Complete
     */
    void complete() {
        if (!mFifoFile.is_open() || mHasCompleted) {
            return;
        }
        int64_t updateRest = mTotal - mTotalUpdateAmount + mNextUpdateAmount;
        update(updateRest, true);
        std::string toSend = constructStatusCmd(mkPid, "Complete!");
        mFifoFile.write(toSend.c_str(), (long)toSend.length());
        mFifoFile.flush();
        // Completion guard
        mHasCompleted = true;
        mRunning.store(false, std::memory_order_relaxed);
    }

    std::string getCurrentPipePath() {
        if (!mFifoFile.is_open()) {
            return "";
        }
        return mkFifoPath;
    }

  private:
    /**
     * @brief Indicates abnormal exit of a program. Keep progress to its
     * current reading and change the status to Terminated.
     */
    void terminate() {
        if (!mFifoFile.is_open() || mHasCompleted) {
            return;
        }
        update(mNextUpdateAmount, true);
        std::string toSend = constructStatusCmd(mkPid, "Terminated");
        mFifoFile.write(toSend.c_str(), (long)toSend.length());
        mFifoFile.flush();
        mRunning.store(false, std::memory_order_relaxed);
    }
    /**
     * @brief Construct the text-based INIT command sent to the monitor
     *
     * @param pid Process id as the identifier
     * @param desc Description message to be shown besides the progress bar
     * @param total Total iterations to be shown in the progress bar
     */
    static std::string constructInitCmd(pid_t pid, const std::string& desc,
                                        int64_t total) {
        std::ostringstream oss;
        oss << "{ \"pid\": " << pid;
        oss << ", \"total\": " << total;
        oss << ", \"desc\": \"" << desc << "\"";
        oss << ", \"status\": \"Running\"";
        oss << " }\n";
        assert(oss.str().length() < PIPE_BUF);
        return oss.str();
    }

    /**
     * @brief Construct the text-based UPDATE command sent to the monitor
     *
     * @param pid Process id as the identifier
     * @param n Num of iterations to be incremented in the progress bar
     */
    static std::string constructUpdateCmd(pid_t pid, int64_t updateAmount) {
        std::ostringstream oss;
        oss << "{ \"pid\": " << pid;
        if (updateAmount != 1) {
            oss << ", \"n\": " << updateAmount;
        }
        oss << " }\n";
        assert(oss.str().length() < PIPE_BUF);
        return oss.str();
    }

    /**
     * @brief Construct the text-based STATUS command sent to the monitor
     *
     * @param pid Process id as the identifier
     * @param desc Description message to be shown besides the progress bar
     */
    static std::string constructStatusCmd(pid_t pid,
                                          const std::string& newStatus) {
        std::ostringstream oss;
        oss << "{ \"pid\": " << pid;
        oss << ", \"status\": \"" << newStatus << "\"";
        oss << " }\n";
        assert(oss.str().length() < PIPE_BUF);
        return oss.str();
    }

    const pid_t mkPid;          // Process id
    int64_t mTotal;             // Max progress amount
    int64_t mTotalUpdateAmount; // Accumulated progress amount
    int64_t mNextUpdateAmount;  // Amount to update at the next update call
    const float mkMinInterval;  // Unit: second
    std::chrono::time_point<std::chrono::high_resolution_clock>
      mLastUpdate; // ? I spare a lock here, but should not be a problem
    std::atomic_bool mRunning;

    bool mHasInitialized; // Has been initialized once
    bool mHasCompleted;   // Has been completed normally

    const std::string mkFifoPath; // Name of the communication pipe
    std::ofstream mFifoFile;      // Handle of the communication handle
};

#endif /* ATQDM_H_ */
