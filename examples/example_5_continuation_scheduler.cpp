#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include <thread>
#include <iostream>
#include <chrono>

exec::timed_thread_context timer;

auto coroutine(std::string name) -> exec::task<void> {
    std::cout << name << " tid: " << std::this_thread::get_id() << '\n';

    co_await exec::schedule_after(timer.get_scheduler(), std::chrono::milliseconds(100));

    std::cout << name << " after tid: " << std::this_thread::get_id() << '\n';

}

int main() {
    exec::single_thread_context ctx;
    exec::async_scope scope;
    stdexec::sync_wait(coroutine("sync_wait"));


    scope.spawn(coroutine("spawn bare"));

    stdexec::sync_wait(scope.on_empty());

    scope.spawn(stdexec::starts_on(ctx.get_scheduler(), coroutine("spawn with starts_on")));

    stdexec::sync_wait(scope.on_empty());

    scope.spawn(stdexec::schedule(ctx.get_scheduler()) | stdexec::let_value([]() { return coroutine("schedule and let_value"); }));

    stdexec::sync_wait(scope.on_empty());

    return 0;
}