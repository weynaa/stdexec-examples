#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include <thread>
#include <iostream>
#include <chrono>


exec::single_thread_context otherThread;
exec::timed_thread_context timer;

auto coroutine(int number, int c) -> exec::task<void> {
    for (auto i = 0; i < c; ++i) {
        co_await exec::schedule_after(timer.get_scheduler(),
            std::chrono::seconds(1));
        std::cout << "in coroutine " << number
            << ' ' << i
            << ' ' <<
            std::this_thread::get_id() << '\n';
    }

}

int main() {
    exec::async_scope scope;
    scope.spawn(stdexec::starts_on(
        otherThread.get_scheduler(), coroutine(0, 3)));
    scope.spawn(coroutine(1, 10));

    stdexec::sync_wait(exec::schedule_after(timer.get_scheduler(),
        std::chrono::seconds(3)));
    scope.request_stop();

    stdexec::sync_wait(scope.on_empty());

    return 0;
}