#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>

#include <thread>
#include <iostream>

int main() {
    exec::single_thread_context other_thread;
    stdexec::sync_wait(
        stdexec::just()
        | stdexec::then([]() {
            std::cout << "start on main thread_id " << std::this_thread::get_id() << '\n';
            })
        | stdexec::continues_on(other_thread.get_scheduler())
        | stdexec::then([]() {
            std::cout << "continue on other thread_id " << std::this_thread::get_id() << '\n';
            })
    );
    std::cout << "\n\n";
    stdexec::sync_wait(
        stdexec::just()
        | stdexec::then([]() {
            std::cout << "start on main thread_id " << std::this_thread::get_id() << '\n';
            })
        | stdexec::let_value([&]() {
            return stdexec::schedule(other_thread.get_scheduler()) | stdexec::then([]() {
                std::cout << "continue on other thread_id " << std::this_thread::get_id() << '\n';
                });
            })
    );

    return 0;
}