#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include <thread>
#include <iostream>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

exec::timed_thread_context timer;

struct ExampleHardware : std::enable_shared_from_this< ExampleHardware> {
    ~ExampleHardware() {
        scope.request_stop();
        stdexec::sync_wait(scope.on_empty());
    }
    auto sendAndForget(int i) {
        scope.spawn(stdexec::starts_on(timer.get_scheduler(), sendData(i)));
    }
private:
    auto sendData(int i) -> exec::task<void> {
        //Keep object alive until code is finished
        const auto lock = shared_from_this();
        if (!lock) {
            co_return;
        }

        co_await exec::schedule_after(timer.get_scheduler(), 100ms);
        std::cout << "time to do work, value: " << lock->internalNumber;
        lock->internalNumber += i;
        co_await exec::schedule_after(timer.get_scheduler(), 100ms);
        std::cout << "work is done, value: " << lock->internalNumber;
        //DEADLOCK here
    }
    std::atomic<int> internalNumber{ 0 };
    exec::async_scope scope;
};

int main() {
    exec::async_scope scope;
    auto hardwareObject = std::make_shared< ExampleHardware>();
    hardwareObject->sendAndForget(3);
    std::this_thread::sleep_for(10ms);
    //Disconnected, i decide i no longer need this
    hardwareObject.reset();

    return 0;
}