# doublebuf

A simple lock-free double buffering mechanism implementation written in C++20

## Features

This library provides two double buffering mechanism,

- `DoubleBuf`

  Standard double buffering mechanism.

- `LazyDoubleBuf`

  Lazily updated double buffering mechanism. This buffering is akin to triple buffering where there will be fewer updates done into the back buffer: the update to the back buffer is only done if the data is swapped by the consumer. The difference with triple buffering then is the number of buffer used, with this one, there is only two buffers.

  You can think of this buffering method as triple buffering with its back buffer can be said as the middle buffer of a traditional triple buffering mechanism. Your data that is used to update the back buffer of `LazyDoubleBuf` then is the back buffer of a traditional triple buffering mechanism.

  The recency of this buffer is then as bad as a triple buffering and affected by the rate of the consumer swapping the buffer.

  ```
                                    ╭─────────────────────────────────────────╮
                                    │              LazyDoubleBuf              │
                                    ├─────────────────────────────────────────┤
  ┏━━━━━━━━━━━┓    `update(fn)`     │  ┌─────────────┐       ┌──────────────┐ │
  ┃ Producer  ┃───────────────────────▶│ back_buffer │◀─────▶│ front_buffer │ │
  ┗━━━━━━━━━━━┛ │ only called if  │ │  └─────────────┘   ▲   └──────────────┘ │
                │swap has happened│ ╰────────────────────│────────────────────╯
                                                         │
                                                `swap()` │ │swap might not happen│
                                                         │ │if data is not ready │
                                                         │
                                                    ┏━━━━━━━━━━┓
                                                    ┃ Consumer ┃
                                                    ┗━━━━━━━━━━┛
  ```
## Usage

> `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
  doublebuf
  GIT_REPOSITORY https://github.com/mrizaln/doublebuf
  GIT_TAG v0.2.0)
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

using doublebuf::LazyDoubleBuf;

template <typename... Ts>
void println(std::format_string<Ts...>&& fmt, Ts&&... ts)
{
    auto string = std::format(fmt, std::forward<Ts>(ts)...);
    std::cout << std::format("{}\n", string) << std::flush;
}

int main()
{
    using namespace std::chrono_literals;

    auto buf = LazyDoubleBuf<std::string>{ "front", "back" };

    // producer thread
    std::jthread producer([&buf](const std::stop_token& st) {
        int counter = 0;
        while (!st.stop_requested()) {

            /* some work... */

            ++counter;
            println("producer: counter: {}", counter);

            // will be called called only if the buffer is idle (after swap)
            auto update = buf.update_buffers([&counter](std::string& buffer) {
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

        // front buffer guaranteed to be free to use after this until the next call to swap_buffers()
        auto&& [buffer, swapped] = buf.swap_buffers();

        // swap_buffers() may not swap anything and return false flag to indicate that
        if (not swapped) {
            // do something
        }

        /* using the front buffer */

        println("consumer: (S) buffer: {}", buffer);

        std::this_thread::sleep_for(1s);
    };

    println("{} has passed, time to stop...", duration);

    producer.request_stop();
}
```
