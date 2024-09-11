# doublebuf

A simple lock-free double buffering mechanism implementation written in C++20

## Usage

> `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
  doublebuf
  GIT_REPOSITORY https://github.com/mrizaln/doublebuf
  GIT_TAG main)
FetchContent_MakeAvailable(doublebuf)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE doublebuf)
```

## Example

> main.cpp

```cpp
#include <doublebuf/double_buffer_atomic.hpp>

#include <chrono>
#include <format>
#include <iostream>
#include <thread>

using doublebuf::DoubleBufferAtomic;

template <typename... Ts>
void println(std::format_string<Ts...>&& fmt, Ts&&... ts)
{
    auto string = std::format(fmt, std::forward<Ts>(ts)...);
    std::cout << std::format("{}\n", string) << std::flush;
}

int main()
{
    using namespace std::chrono_literals;

    auto buf = DoubleBufferAtomic<std::string>{ "front", "back" };

    // producer thread
    std::jthread producer([&buf](const std::stop_token& st) {
        int counter = 0;
        while (!st.stop_requested()) {

            /* some work... */

            ++counter;
            println("producer: counter: {}", counter);

            // will be processed if the buffer is idle (after swap)
            auto update = buf.updateBuffers([&counter](std::string& buffer) {
                buffer = std::format("{0} ==> {0:032b}", counter);
                println("producer: [U] buffer: {}", buffer);
            });

            if (update) {
                // do some logic if data sent to back buffer (lambda called)
            } else {
                // do some logic if data is not sent to back buffer (lambda ignored)
            }

            std::this_thread::sleep_for(87ms);
        }
    });

    // consumer thread (this thread)

    using Clock = std::chrono::steady_clock;

    auto duration = 10s;
    auto start    = Clock::now();

    while (Clock::now() < start + duration) {

        /* some work... */

        // front buffer guaranteed to be free to use after this until the next call to swapBuffers
        auto&& [buffer, swapped] = buf.swapBuffers();

        /* using the front buffer */

        println("consumer: (S) buffer: {}", buffer);

        std::this_thread::sleep_for(1s);
    };

    println("{} has passed, time to stop...", duration);

    producer.request_stop();
}
```
