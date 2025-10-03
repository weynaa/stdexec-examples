#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <exec/task.hpp>

#include <thread>
#include <iostream>
#include <chrono>


exec::single_thread_context thread_context;
exec::timed_thread_context timer;

auto coroutine2(int i) -> exec::task<void> {
    co_await exec::schedule_after(timer.get_scheduler(), std::chrono::seconds(1));
    std::cout << "in coroutine2 " << i << ' ' << std::this_thread::get_id() << '\n';
}

auto coroutine() -> exec::task<void>{
    std::cout << "in coroutine " << std::this_thread::get_id() << '\n';
    for (int i = 0; i < 5; ++i) {
        co_await coroutine2(i);
    }
}

int main() {
    std::cout << "main thread id " << std::this_thread::get_id() << '\n';
    stdexec::sync_wait(
        coroutine()
    );

    return 0;
}