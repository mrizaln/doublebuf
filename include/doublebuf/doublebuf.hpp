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
    template <std::movable T, bool DynamicAlloc = false>
    class DoubleBuf
    {
    public:
        using Value         = T;
        using UnderlyingBuf = std::conditional_t<DynamicAlloc, std::unique_ptr<T[]>, std::array<T, 2>>;

        static bool constexpr is_dynamic_alloc    = DynamicAlloc;
        static bool constexpr is_always_lock_free = std::atomic<std::uint32_t>::is_always_lock_free;

        enum class BufUpdateStatus : std::uint32_t
        {
            Idle,
            Updating,
            Done,
        };

        struct [[nodiscard]] SwapResult
        {
            Value& m_buffer;
            bool   m_swapped;
        };

        explicit DoubleBuf(Value front = {}, Value back = {})
            requires (!DynamicAlloc)
            : m_buffers{ std::move(front), std::move(back) }
        {
        }

        explicit DoubleBuf(Value front = {}, Value back = {})
            requires (DynamicAlloc)
            : m_buffers{ new Value[2]{ std::move(front), std::move(back) } }
        {
        }

        SwapResult swap_buffers() noexcept
        {
            if (m_info != BufUpdateStatus::Done) {
                return { m_buffers[m_front], false };
            }

            // the index swapped here
            auto front = m_front.fetch_xor(1) ^ 1;    // emulate xor_fetch

            m_info = BufUpdateStatus::Idle;

            return { m_buffers[front], true };
        }

        bool update_buffers(std::invocable<Value&> auto&& update) noexcept
        {
            if (m_info != BufUpdateStatus::Idle) {
                return false;
            }
            m_info = BufUpdateStatus::Updating;

            auto back = m_front ^ 1;    // access the back buffer
            std::forward<decltype(update)>(update)(m_buffers[back]);

            m_info = BufUpdateStatus::Done;
            return true;
        }

        Value&       front() noexcept { return m_buffers[m_front]; }
        const Value& front() const noexcept { return m_buffers[m_front]; }

        Value&       back() noexcept { return m_buffers[m_front ^ 1]; }
        const Value& back() const noexcept { return m_buffers[m_front ^ 1]; }

        BufUpdateStatus status() const noexcept { return m_info; }

    private:
        UnderlyingBuf                m_buffers;
        std::atomic<BufUpdateStatus> m_info  = BufUpdateStatus::Idle;
        std::atomic<std::uint32_t>   m_front = 0;
    };

    // NOTE: if the following static_assert fails, the atomic operations are not lock-free, you may want to
    // disable this check and allow the code to compile, but be aware that the performance may be affected

#if not defined(DOUBLEBUF_DISABLE_LOCKFREE_CHECK)
    // lock-free requirements for atomic operations
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
#endif
}

#endif /* end of include guard: DOUBLEBUF_DOUBLEBUF_HPP_T4O73RT39J */
