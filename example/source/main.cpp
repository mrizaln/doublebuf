#include <doublebuf/doublebuf.hpp>

#include <csignal>
#include <format>
#include <string>
#include <thread>

#define DOUBLEBUF_TEST_NO_SLEEP 1

#if DOUBLEBUF_TEST_NO_SLEEP
#    define doublebuf_sleep(arg)
#else
#    define doublebuf_sleep(arg) std::this_thread::sleep_for(arg)
#endif

std::atomic<bool> g_interrupt = false;

// No println in C++20 yet
template <typename... Args>
void println(std::format_string<Args...> fmt, Args&&... args)
{
    std::puts(std::format(fmt, std::forward<Args>(args)...).c_str());
}

int main()
{
    using namespace std::chrono_literals;

    using Buffer    = std::string;
    using DoubleBuf = doublebuf::LazyDoubleBuf<Buffer, false>;    // on the stack (std::array)
    // using DoubleBuf = DoubleBuf<Buffer, true>;     // on the heap (std::unique_ptr<Buffer[]>)

    std::signal(SIGINT, [](int sig) {
        std::puts("Interrupt signal received. Exiting...");
        g_interrupt.store(true);
        g_interrupt.notify_all();
        std::signal(sig, SIG_DFL);
    });

    auto db = DoubleBuf{ "front", "back" };

    // clang-format off
    println("sizeof DoubleBuf<Buffer>           = {}", sizeof(DoubleBuf));
    println("sizeof Buffer [array] (dyn: {:<5}) = {}", DoubleBuf::is_dynamic_alloc, sizeof(DoubleBuf::Buf));
    println("sizeof Buffer                      = {}", sizeof(DoubleBuf::Value));
    // clang-format on

    println("front: {}", db.front());    // no synchronization on access
    println("back : {}", db.back());     // no synchronization on access

    // producer thread
    auto producer = std::jthread{ [&db](const std::stop_token& st) {
        int counter  = 0;
        while (!st.stop_requested()) {

            /* pretend to do some work */

            // will be called if the buffer is idle (after swap)
            auto update = db.update([&counter](Buffer& buffer) {
                buffer = std::format("{0} ==> {0:032b}", counter);
            });

            if (update) {
                println("producer: [U] buffer: {}", counter);
            }

            ++counter;
            println("producer: counter: {}", counter);
            doublebuf_sleep(134ms);
        }
    } };

    // consumer thread
    auto consumer = std::jthread{ [&db](const std::stop_token& st) {
        while (!st.stop_requested()) {

            /* pretend to do some work */

            // guaranteed to be free to use after call to swapBuffers and before the next call to swapBuffers
            auto&& [buffer, swapped] = db.swap();

            println("consumer: (S: {:<5}) buffer: {}", swapped, buffer);
            doublebuf_sleep(1078ms);
        }
    } };

    // // second consumer thread (unsynchronized access to the buffer): DON'T DO THIS
    // auto t3 = std::jthread{ [&db](const std::stop_token& st) {
    //     while (!st.stop_requested()) {
    //         /* pretend to do some work */

    //         const auto& buffer = db.front();

    //         // unsynchronized access: the buffer might be swapped while read
    //         println("t3: (F) buffer: {}", buffer);
    //         std::this_thread::sleep_for(108ms);
    //     }
    // } };

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
