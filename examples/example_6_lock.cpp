#include <stdexec/execution.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/timed_thread_scheduler.hpp>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include <chrono>

exec::timed_thread_context timer;

auto coroutine() -> exec::task<void> {
  std::mutex m;
  std::unique_lock l(m);
  co_await exec::schedule_after(timer.get_scheduler(), std::chrono::milliseconds(100));
}

int main() {
  exec::async_scope scope;

  scope.spawn(coroutine());

  stdexec::sync_wait(scope.on_empty());
  return 0;
}
