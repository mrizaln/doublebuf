#ifndef DOUBLEBUF_DOUBLEBUF_HPP_T4O73RT39J
#define DOUBLEBUF_DOUBLEBUF_HPP_T4O73RT39J

#include <array>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace doublebuf
{
    // NOTE: if the following static_assert fails, the atomic operations are not lock-free, but if you want
    // to use atomics regardless, you might want to #define DOUBLEBUF_DISABLE_LOCKFREE_CHECK macro

#if not defined(DOUBLEBUF_DISABLE_LOCKFREE_CHECK)
    // lock-free requirements for atomic operations
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
#endif

    enum class BufStatus : std::uint32_t
    {
        Idle,
        Done,
    };

    template <typename T>
    struct [[nodiscard]] SwapResult
    {
        T&   m_buffer;
        bool m_swapped;
    };

    /**
     * @class LazyDoubleBuf
     *
     * @brief Deferred-update double buferring mechanism
     *
     * This double buffering mechanism is akin to triple buffering where the back buffer of this buffer is
     * the middle buffer of the triple buffer. The back buffer will only be updated if the swap is made
     * beforehand on the condition called Idle (container starts in Idel state). After update is done the
     * state then transitioned to Done on which the next swap will actually swap the buffer otherwise the
     * actual swap won't be done. The cycle continues.
     *
     * By this mechanism, the consequence to the data stored is that the data swapped may be not the most
     * updated data. The data will lags the back buffer writer since it only updates after a swap is made.
     * Thus the recency of data is affected by the rate front buffer accessor swapping the buffers. This is
     * very akin to triple buffering.
     */
    template <std::movable T, bool DynamicAlloc = false>
    class LazyDoubleBuf
    {
    public:
        using Value = T;
        using Buf   = std::conditional_t<DynamicAlloc, std::unique_ptr<T[]>, std::array<T, 2>>;

        static constexpr bool is_dynamic_alloc    = DynamicAlloc;
        static constexpr bool is_always_lock_free = std::atomic<std::uint32_t>::is_always_lock_free;

        /**
         * @brief Construct LazyDoubleBuf using default value for the buffer.
         */
        LazyDoubleBuf()
            requires std::default_initializable<Value>
            : LazyDoubleBuf{ Value{}, Value{} }
        {
        }

        /**
         * @brief Construct LazyDoubleBuf using the provided value for the buffer.
         *
         * @param front The front buffer value.
         * @param back The back buffer value.
         */
        LazyDoubleBuf(Value front, Value back)
            requires (not DynamicAlloc)
            : m_buffers{ std::move(front), std::move(back) }
        {
        }

        /**
         * @brief Construct LazyDoubleBuf using the provided value for the buffer.
         *
         * @param front The front buffer value.
         * @param back The back buffer value.
         */
        LazyDoubleBuf(Value front, Value back)
            requires (DynamicAlloc)
            : m_buffers{ new Value[2]{ std::move(front), std::move(back) } }
        {
        }

        /**
         * @brief Swaps the front and back buffer.
         *
         * @return Return the front buffer and whether the swap is actually done.
         */
        SwapResult<Value> swap() noexcept
        {
            if (m_info.load(Ord::acquire) != BufStatus::Done) {
                return { m_buffers[m_front.load(Ord::relaxed)], false };
            }

            // m_front is not used to synchronize the access to the buffers, so we can use relaxed order
            auto front = m_front.fetch_xor(1, Ord::relaxed) ^ 1;    // emulate xor_fetch

            m_info.store(BufStatus::Idle, Ord::release);

            return { m_buffers[front], true };
        }

        /**
         * @brief Update the back buffer
         *
         * @param update The update function to be called on the back buffer
         * @return Return true if the update is done, false otherwise
         */
        bool update(std::invocable<Value&> auto&& update) noexcept
        {
            if (m_info.load(Ord::acquire) != BufStatus::Idle) {
                return false;
            }

            auto back = m_front.load(Ord::relaxed) ^ 1;    // access the back buffer
            std::forward<decltype(update)>(update)(m_buffers[back]);

            m_info.store(BufStatus::Done, Ord::release);
            return true;
        }

        /**
         * @brief Access the front buffer (unsafe; synchronization is up to the user)
         */
        Value& front() noexcept { return m_buffers[m_front.load(Ord::relaxed)]; }

        /**
         * @brief Access the front buffer (unsafe; synchronization is up to the user)
         */
        const Value& front() const noexcept { return m_buffers[m_front.load(Ord::relaxed)]; }

        /**
         * @brief Access the back buffer (unsafe; synchronization is up to the user)
         */
        Value& back() noexcept { return m_buffers[m_front.load(Ord::relaxed) ^ 1]; }

        /**
         * @brief Access the back buffer (unsafe; synchronization is up to the user)
         */
        const Value& back() const noexcept { return m_buffers[m_front.load(Ord::relaxed) ^ 1]; }

    private:
        using Ord = std::memory_order;

        Buf                        m_buffers;
        std::atomic<BufStatus>     m_info  = BufStatus::Idle;
        std::atomic<std::uint32_t> m_front = 0;
    };
}

#endif /* end of include guard: DOUBLEBUF_DOUBLEBUF_HPP_T4O73RT39J */
