#include <exec/async_scope.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/task.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <mutex>
#include <stdexec/execution.hpp>

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include <gtest/gtest.h>
#include <iostream>

using namespace std::chrono_literals;

// We don't need a lock on this, it is only accessed in the harware thread
exec::timed_thread_context internal_hardware_thread;

constexpr float TEMP_THRESHOLD = 60;
struct MotorData
{

    float temparature { 20 };
    float position { 20 };

    bool error { false };
};

// Mockup hardware interface
struct MotorCommunication
{
    explicit MotorCommunication(std::shared_ptr<MotorData> data) noexcept
        : m_data(std::move(data))
    {
    }

    /**
     * @brief subscribe to information about the hardware
     * @param callback  will be called periodically with the current temperature
     * @return exec::task<void> will never end, but it can be stopped to stop
     * the subscription. It will fail with an error when the communication to
     * the device failed
     */
    [[nodiscard]]
    auto subscribeSensor(std::function<exec::task<void>(float)> callback)
        -> exec::task<void>
    {
        while (true) {
            auto [error, temp] = co_await (
                exec::schedule_after(
                    internal_hardware_thread.get_scheduler(),
                    200ms)
                | stdexec::then([this]() {
                      return std::make_tuple(
                          m_data->error,
                          m_data->temparature);
                  }));
            if (error) {
                throw std::runtime_error(
                    "subscribeSensor: communication_error");
            }
            co_await callback(temp);
        }
    }

    /**
     * @brief Turn motor
     * @param position, between 0 and 360 degrees per second
     */
    auto turnMotor(const float position) -> exec::task<void>
    {
        if (position < 0 || position > 360) {
            throw std::runtime_error("turn: invalid desitination position");
        }

        co_await (
            exec::schedule_after(
                internal_hardware_thread.get_scheduler(),
                500ms)
            | stdexec::then([=]() {
                  if (m_data->error) {
                      throw std::runtime_error("turn: communication error");
                  }
                  if (m_data->temparature > TEMP_THRESHOLD) {
                      throw std::runtime_error("turn: temperature exceeded");
                  }
                  m_data->position = position;
              }));
    }

private:
    std::shared_ptr<MotorData> m_data;
};

bool cloudError { false };
std::vector<std::string> loggedErrors;

auto uploadMessageToCloud(std::string message) -> exec::task<void>
{
    if (cloudError) {
        throw std::runtime_error("message upload to cloud failed");
    }
    // It is a really bad connection
    co_return co_await (
        exec::schedule_after(internal_hardware_thread.get_scheduler(), 200ms)
        | stdexec::then([=]() {
              if (cloudError) {
                  throw std::runtime_error("message upload to cloud failed");
              }
              loggedErrors.push_back(message);
          }));
}

bool ledError { false };
std::vector<float> ledChanges { 0.f };

auto setLedPower(const float power) -> exec::task<void>
{
    if (power < 0 || power > 1) {
        throw std::runtime_error("invalid led power value");
    }
    if (ledError) {
        throw std::runtime_error("set led power failed");
    }
    // It is a really bad connection
    co_return co_await (
        exec::schedule_after(internal_hardware_thread.get_scheduler(), 200ms)
        | stdexec::then([=]() {
              if (ledError) {
                  throw std::runtime_error("set led power failed");
              }
              ledChanges.push_back(power);
          }));
}

// You are free to use this
exec::timed_thread_context timer;
exec::single_thread_context single_threadContext;

struct HardwareManager
{
    enum class Status
    {
        Ok,
        Overtemperature,
        CommunicationError
    };
    explicit HardwareManager(
        MotorCommunication &motor,
        std::function<void(Status)> statusChangedCallback)
        : m_motor(motor)
        , m_statusChangedCallback(statusChangedCallback)
    {
        // Initialize
    }

    ~HardwareManager()
    {
        //Cleanup
    }

    auto turn(const float position) -> exec::task<void>
    {
        // turn
        co_return;
    }

    auto setStatus(Status status) -> bool
    {
        // just a helper for you
        std::unique_lock l(m_mutex);
        if (m_status == status) {
            return false;
        }
        m_status = status;
        l.unlock();
        if (m_statusChangedCallback) {
            m_statusChangedCallback(m_status);
        }
        return true;
    }

private:

    const std::function<void(Status)> m_statusChangedCallback;
    MotorCommunication &m_motor;
    Status m_status;
    std::mutex m_mutex;
};

template <class F>
auto checkSafe(F &&f)
{
    stdexec::sync_wait(
        stdexec::schedule(internal_hardware_thread.get_scheduler())
        | stdexec::then(std::forward<F>(f)));
}

auto getErrorBefore() -> size_t
{
    auto val = stdexec::sync_wait(
        stdexec::schedule(internal_hardware_thread.get_scheduler())
        | stdexec::then([]() { return loggedErrors.size(); }));
    return std::get<0>(*val);
}

int main()
{
    auto data = std::make_shared<MotorData>();

    MotorCommunication comm(data);

    std::vector<HardwareManager::Status> statusChanges;
    HardwareManager manager(comm, [&](HardwareManager::Status status) {
        statusChanges.push_back(status);
    });

    // Test 1: Normal motor turn operation
    std::cout << "Test 1: Normal motor turn operation\n";
    stdexec::sync_wait(manager.turn(120));
    checkSafe([&]() {
        EXPECT_EQ(data->position, 120);
        EXPECT_EQ(loggedErrors.size(), 0);
        ASSERT_EQ(ledChanges.size(), 3);
        EXPECT_EQ(ledChanges[0], 0.f); // Initial value
        EXPECT_EQ(ledChanges[1], 1.f); // LED turned on
        EXPECT_EQ(ledChanges[2], 0.f); // LED turned off
        ledChanges.push_back(0.f); // Reset to initial state
    });

    // Test 2: Overtemperature detection
    std::cout << "Test 2: Overtemperature detection\n";
    data->temparature = TEMP_THRESHOLD + 10;
    std::this_thread::sleep_for(500ms); // Wait for sensor monitoring to detect
    checkSafe([&]() {
        EXPECT_GT(statusChanges.size(), 0);
        EXPECT_EQ(
            statusChanges.back(),
            HardwareManager::Status::Overtemperature);
        ASSERT_GT(loggedErrors.size(), 0);
        EXPECT_EQ(loggedErrors.back(), "Overtemperature encountered");
    });

    // Test 3: Motor turn fails when overtemperature
    std::cout << "Test 3: Motor turn fails when overtemperature\n";
    size_t errorCountBefore = getErrorBefore();
    EXPECT_THROW(stdexec::sync_wait(manager.turn(200)), std::runtime_error);
    checkSafe([&]() {
        EXPECT_NE(data->position, 200); // Position should not change
        EXPECT_GT(
            loggedErrors.size(),
            errorCountBefore); // Error should be logged
    });

    // Test 4: Recovery from overtemperature
    std::cout << "Test 4: Recovery from overtemperature\n";
    checkSafe([&]() { data->temparature = TEMP_THRESHOLD - 10; });
    std::this_thread::sleep_for(500ms); // Wait for sensor monitoring
    EXPECT_EQ(statusChanges.back(), HardwareManager::Status::Ok);

    // Test 5: Motor turn succeeds after temperature recovery
    std::cout << "Test 5: Motor turn succeeds after temperature recovery\n";
    stdexec::sync_wait(manager.turn(200));
    checkSafe([&]() { EXPECT_EQ(data->position, 200); });

    // Test 6: Communication error handling
    std::cout << "Test 6: Communication error handling\n";
    checkSafe([&]() { data->error = true; });
    std::this_thread::sleep_for(500ms); // Wait for sensor monitoring
    checkSafe([&]() {
        bool foundCommError = false;
        for (const auto &status : statusChanges) {
            if (status == HardwareManager::Status::CommunicationError) {
                foundCommError = true;
                break;
            }
        }
        EXPECT_TRUE(foundCommError);
    });

    // Test 7: Motor turn fails with communication error
    std::cout << "Test 7: Motor turn fails with communication error\n";
    size_t errorCountBefore2 = getErrorBefore();
    EXPECT_THROW(stdexec::sync_wait(manager.turn(250)), std::runtime_error);
    checkSafe([&]() {
        EXPECT_NE(data->position, 250); // Position should not change
        EXPECT_GT(
            loggedErrors.size(),
            errorCountBefore2); // Error should be logged
    });

    // Test 8: LED error handling (cloud upload should still work)
    std::cout << "Test 8: LED error handling\n";
    checkSafe([&]() {
        data->error = false;
        data->temparature = 20;
        ledError = true;
    });
    size_t errorCountBefore3 = getErrorBefore();
    EXPECT_THROW(stdexec::sync_wait(manager.turn(300)), std::runtime_error);
    std::this_thread::sleep_for(300ms);
    checkSafe([&]() {
        EXPECT_NE(data->position, 300); // Turn fails due to LED error
        EXPECT_GT(
            loggedErrors.size(),
            errorCountBefore3); // Error logged to cloud
        ledError = false;
        cloudError = true;
        data->temparature = TEMP_THRESHOLD + 5;
    });

    // Test 9: Cloud error handling (should be suppressed)
    std::cout << "Test 9: Cloud error handling\n";

    std::this_thread::sleep_for(500ms);
    // Should not crash, error is suppressed

    std::cout << "All tests completed successfully!\n";

    return 0;
}
