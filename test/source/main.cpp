#include <doublebuf/double_buffer_atomic.hpp>

#include <fmt/core.h>

#include <csignal>
#include <string>
#include <thread>

std::atomic<bool> g_interrupt = false;

int main()
{
    using namespace std::chrono_literals;

    using Buffer       = std::string;
    using DoubleBuffer = DoubleBufferAtomic<Buffer, false>;    // on the stack (std::array)
    // using DoubleBuffer = DoubleBufferAtomic<Buffer, true>;   // on the heap (std::unique_ptr<Buffer[]>)

    std::signal(SIGINT, [](int sig) {
        std::puts("Interrupt signal received. Exiting...");
        g_interrupt.store(true);
        g_interrupt.notify_all();
        std::signal(sig, SIG_DFL);
    });

    auto db = DoubleBuffer{ "start" };

    // clang-format off
    fmt::println("sizeof DoubleBufferAtomic<Buffer>  = {}", sizeof(DoubleBuffer));
    fmt::println("sizeof Buffer [array] (dyn: {:<5}) = {}", DoubleBuffer::s_dynamicAlloc, sizeof(DoubleBuffer::BuffersType));
    fmt::println("sizeof Buffer                      = {}", sizeof(DoubleBuffer::BufferType));
    fmt::println("sizeof BufferUpdateStatus          = {}", sizeof(DoubleBuffer::BufferUpdateStatus));
    // clang-format on

    // producer thread
    std::jthread producer([&db](const std::stop_token& st) {
        int counter = 0;
        while (!st.stop_requested()) {

            /* pretend to do some work */

            db.updateBuffer([&counter](Buffer& buffer) {
                std::this_thread::sleep_for(239ms);
                buffer = fmt::format("{0} ==> {0:032b}", counter);
                fmt::println("producer: [U] buffer: {}", buffer);
            });

            ++counter;
            fmt::println("producer: counter: {}", counter);
            std::this_thread::sleep_for(134ms);
        }
    });

    // consumer thread
    std::jthread consumer([&db](const std::stop_token& st) {
        while (!st.stop_requested()) {

            /* pretend to do some work */

            const auto& buffer = db.swapBuffers();

            fmt::println("consumer: (S) buffer: {}", buffer);
            std::this_thread::sleep_for(1078ms);
        }
    });

    // // second consumer thread (unsynchronized access to the buffer): DON'T DO THIS
    // std::jthread t3([&db](const std::stop_token& st) {
    //     while (!st.stop_requested()) {
    //         /* pretend to do some work */

    //         const auto& buffer = db.getFront();

    //         // unsynchronized access: the buffer might be swapped while read
    //         fmt::println("t3: (F) buffer: {}", buffer);
    //         std::this_thread::sleep_for(108ms);
    //     }
    // });

    // multiple consumer might be possible with sub-consumer like following:
    // - consumer: swaps the buffer
    // - consumer: spawns (sync or async) sub-consumers
    //      - sub-consumer 1: reads the buffer
    //      - sub-consumer 2: reads the buffer
    //      - sub-consumer 3: reads the buffer
    //      - ...
    // - consumer: waits for all sub-consumers to finish (if async, possibly with a barrier)
    // - consumer: continues with the next swap

    g_interrupt.wait(false);

    return 0;
}
