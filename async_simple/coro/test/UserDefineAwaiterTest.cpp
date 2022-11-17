/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <chrono>
#include <thread>
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "async_simple/test/unittest.h"

namespace async_simple {
namespace coro {

class UserDefineAwaiterTest : public FUTURE_TESTBASE {
public:
    UserDefineAwaiterTest() = default;
    void caseSetUp() override {}
    void caseTearDown() override {}
};

TEST_F(UserDefineAwaiterTest, testWithUserDefineAwaiter) {
    executors::SimpleExecutor e(1);
    struct AwaiterNotHascoAwait {
        AwaiterNotHascoAwait() {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> handle) {
            auto func = [handle]() mutable {
                std::cout << "awaiter work thread id:"
                          << std::this_thread::get_id() << std::endl;
                handle.resume();
            };
            std::thread(std::move(func)).detach();
        }
        void await_resume() {}
    };
    auto lazy1 = [&]() -> Lazy<> {
        auto start_thread_id = std::this_thread::get_id();
        std::cout << "User Define Awaiter no coAwait(start):" << start_thread_id
                  << std::endl;
        co_await AwaiterNotHascoAwait{};
        auto end_thread_id = std::this_thread::get_id();
        std::cout << "User Define Awaiter no coAwait(end):" << end_thread_id
                  << std::endl;
        EXPECT_EQ(start_thread_id, end_thread_id);
    };
    syncAwait(lazy1().via(&e));
    std::cout << "-----------------------------" << std::endl;
    struct AwaiterHascoAwait {
        AwaiterHascoAwait() {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> handle) {
            auto func = [handle]() mutable {
                std::cout << "awaiter work thread id:"
                          << std::this_thread::get_id() << std::endl;
                handle.resume();
            };
            std::thread(std::move(func)).detach();
        }
        auto coAwait(async_simple::Executor *executor) noexcept {
            return std::move(*this);
        }
        void await_resume() {}
    };
    EXPECT_TRUE(detail::HasCoAwaitMethod<AwaiterHascoAwait>);
    auto lazy2 = [&]() -> Lazy<> {
        auto start_thread_id = std::this_thread::get_id();
        std::cout << "User Define Awaiter has coAwait(start):"
                  << start_thread_id << std::endl;
        co_await AwaiterHascoAwait{};
        auto end_thread_id = std::this_thread::get_id();
        std::cout << "User Define Awaiter has coAwait(end):" << end_thread_id
                  << std::endl;
        EXPECT_NE(start_thread_id, end_thread_id);
    };
    syncAwait(lazy2().via(&e));
    std::cout << "-----------------------------" << std::endl;
    struct AwaiterHascoAwaitScheduleByExecutor {
        AwaiterHascoAwaitScheduleByExecutor() {}
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> handle) {
            _ex->schedule([handle]() mutable {
                std::cout << "AwaiterHascoAwaitScheduleByExecutor awaiter work "
                             "thread id:"
                          << std::this_thread::get_id() << std::endl;
                handle.resume();
            });
        }
        auto coAwait(async_simple::Executor *executor) noexcept {
            _ex = executor;
            return std::move(*this);
        }
        void await_resume() {}

        async_simple::Executor *_ex;
    };
    EXPECT_TRUE(detail::HasCoAwaitMethod<AwaiterHascoAwaitScheduleByExecutor>);
    auto lazy3 = [&]() -> Lazy<> {
        auto start_thread_id = std::this_thread::get_id();
        std::cout << "User Define AwaiterHascoAwaitScheduleByExecutor (start):"
                  << start_thread_id << std::endl;
        co_await AwaiterHascoAwaitScheduleByExecutor{};
        auto end_thread_id = std::this_thread::get_id();
        std::cout << "User Define AwaiterHascoAwaitScheduleByExecutor (end):"
                  << end_thread_id << std::endl;
        EXPECT_EQ(start_thread_id, end_thread_id);
    };
    syncAwait(lazy3().via(&e));
}

}  // namespace coro
}  // namespace async_simple
