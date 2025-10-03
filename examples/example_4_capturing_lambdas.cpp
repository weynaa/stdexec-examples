#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include <thread>
#include <iostream>
#include <chrono>

exec::timed_thread_context timer;

void spawnTask(exec::async_scope& scope) {
    auto value_unique = std::make_unique<int>(13);
    scope.spawn([value = std::move(value_unique)]() -> exec::task<void> {
        std::cout << "value is " << *value << " tid: " << std::this_thread::get_id() << '\n';

        co_await exec::schedule_after(timer.get_scheduler(), std::chrono::seconds(1));

        //Value is gone :(
        std::cout << "value is still " << *value << " tid: " << std::this_thread::get_id() << '\n';
    }());
}

int main() {

    exec::async_scope scope;
    spawnTask(scope);

    stdexec::sync_wait(scope.on_empty());

    return 0;
}