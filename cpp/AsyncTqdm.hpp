#ifndef ATQDM_H_
#define ATQDM_H_
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

// File-related variables' naming style I propose:
// For /a/b/c/some.txt
// someDir: /a/b/c
// someFile: the file handle of some.txt
// somePath: /a/b/c/some.txt (most path libraries can directly manipulate)
// someFileName: some.txt

class AsyncTqdm {
  public:
    AsyncTqdm(float minInterval = 0,
              std::string fifoPath = (std::string) "/tmp/" + getlogin() +
                                     (std::string) "/progress.pipe")
        : mPid(getpid())
        , mMinInterval(minInterval)
        , mTotalUpdateAmount(0)
        , mNextUpdateAmount(0)
        , mkFifoPath(std::move(fifoPath))
        , mTotal(0) {
        // Blocking open
        mFifoFile.open(mkFifoPath, std::fstream::out);
    }

    // I do not want to over complicate myself so only inplace use is allowed
    AsyncTqdm(const AsyncTqdm&) = delete;
    AsyncTqdm(AsyncTqdm&&) = delete;
    AsyncTqdm& operator=(const AsyncTqdm&) = delete;
    AsyncTqdm& operator=(AsyncTqdm&&) = delete;
    ~AsyncTqdm() = default;

    void init(const std::string& desc, int32_t total) {
        if (!mFifoFile.is_open()) {
            return;
        }
        mTotal = total;
        std::string toSend = constructInitCmd(mPid, desc, total);
        mFifoFile.write(toSend.c_str(), (long)toSend.length());
        mFifoFile.flush();
    }

    void update(int n = 1, bool immediately = false) {
        if (!mFifoFile.is_open()) {
            return;
        }
        auto now = std::chrono::high_resolution_clock::now();
        auto interval = now - mLastUpdate;
        mLastUpdate = now;
        mTotalUpdateAmount += n;
        mNextUpdateAmount += n;

        constexpr int SEC_TO_NANOSEC = 1e6;
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(interval)
                .count() > long(mMinInterval * SEC_TO_NANOSEC) ||
            immediately) {
            std::string toSend = constructUpdateCmd(mPid, n);
            mFifoFile.write(toSend.c_str(), (long)toSend.length());
            mFifoFile.flush();
            mNextUpdateAmount = 0;
        }
    }

    /**
     * @brief Update the progress to 100% and change the status to Complete
     */
    void complete() {
        if (!mFifoFile.is_open()) {
            return;
        }
        update((int)mTotal - mTotalUpdateAmount + mNextUpdateAmount, true);
        std::string toSend = constructStatusCmd(mPid, "Complete!");
        mFifoFile.write(toSend.c_str(), (long)toSend.length());
        mFifoFile.flush();
    }

    /**
     * @brief Keep progress to its current reading and change the status to
     * Terminated
     */
    void terminate() {
        if (!mFifoFile.is_open()) {
            return;
        }
        std::string toSend = constructStatusCmd(mPid, "Terminated");
        mFifoFile.write(toSend.c_str(), (long)toSend.length());
        mFifoFile.flush();
    }

    std::string getCurrentPipePath() {
        if (!mFifoFile.is_open()) {
            return "";
        }
        return mkFifoPath;
    }

  private:
    /**
     * @brief Construct the text-based INIT command sent to the monitor
     *
     * @param pid Process id as the identifier
     * @param desc Description message to be shown besides the progress bar
     * @param total Total iterations to be shown in the progress bar
     */
    static std::string constructInitCmd(pid_t pid, const std::string& desc,
                                        int32_t total) {
        std::ostringstream oss;
        oss << "{ \"pid\": " << pid;
        oss << ", \"total\": " << total;
        oss << ", \"desc\": \"" << desc << "\"";
        oss << ", \"status\": \"Running\"";
        oss << " }\n";
        return oss.str();
    }

    /**
     * @brief Construct the text-based UPDATE command sent to the monitor
     *
     * @param pid Process id as the identifier
     * @param n Num of iterations to be incremented in the progress bar
     */
    static std::string constructUpdateCmd(pid_t pid, int n) {
        std::ostringstream oss;
        oss << "{ \"pid\": " << pid;
        if (n != 1) {
            oss << ", \"n\": " << n;
        }
        oss << " }\n";
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
        return oss.str();
    }

    pid_t mPid;                  // Process id
    uint32_t mTotal;             // Max progress amount
    uint32_t mTotalUpdateAmount; // Accumulated progress amount
    uint32_t mNextUpdateAmount;  // Amount to update at the next update call
    float mMinInterval;          // Unit: second
    std::chrono::time_point<std::chrono::high_resolution_clock> mLastUpdate;

    const std::string mkFifoPath; // Name of the named pipe
    std::ofstream mFifoFile;
};

#endif /* ATQDM_H_ */
