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

struct ExampleHardware {
    auto sendData(int i) -> exec::task<void> {
        if (!m_connected) {
            throw std::runtime_error("hardware disconnected");
        }
        const auto prev = m_internalNumber.fetch_add(i);
        std::cout << "total sent: " << prev + i;
        co_return;
    }
    std::atomic_bool m_connected{ true };
private:
    std::atomic<int> m_internalNumber{ 0 };
    
};


auto useHardware(ExampleHardware& hardware) -> exec::task<void> {
    if (!hardware.m_connected) {
        co_return;
    }
    co_await hardware.sendData(5);
    co_await exec::schedule_after(timer.get_scheduler(), 100ms);
    co_await hardware.sendData(3);
}

int main() {
    exec::async_scope scope;
    auto hardwareObject = std::make_unique< ExampleHardware>();
    scope.spawn(stdexec::starts_on(timer.get_scheduler(), useHardware(*hardwareObject)));
    std::this_thread::sleep_for(50ms);
    //Disconnected, i decide i no longer need this
    hardwareObject->m_connected = false;
    
    stdexec::sync_wait(scope.on_empty());

    return 0;
}