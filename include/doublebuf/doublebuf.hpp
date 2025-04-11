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
    enum class BufStatus : std::uint32_t
    {
        Idle,
        Updating,
        Done,
    };

    template <typename T>
    struct [[nodiscard]] SwapResult
    {
        T&   m_buffer;
        bool m_swapped;
    };

    template <std::movable T, bool DynamicAlloc = false>
    class DoubleBuf
    {
    public:
        using Value = T;
        using Buf   = std::conditional_t<DynamicAlloc, std::unique_ptr<T[]>, std::array<T, 2>>;

        static constexpr bool is_dynamic_alloc    = DynamicAlloc;
        static constexpr bool is_always_lock_free = std::atomic<std::uint32_t>::is_always_lock_free;

        DoubleBuf()
            requires std::default_initializable<Value>
        = default;

        DoubleBuf(Value front, Value back)
            requires (not DynamicAlloc)
            : m_buffers{ std::move(front), std::move(back) }
        {
        }

        DoubleBuf(Value front, Value back)
            requires (DynamicAlloc)
            : m_buffers{ new Value[2]{ std::move(front), std::move(back) } }
        {
        }

        SwapResult<Value> swap_buffers() noexcept
        {
            if (m_info.load(Ord::acquire) != BufStatus::Done) {
                return { m_buffers[m_front.load(Ord::relaxed)], false };
            }

            // m_front is not used to synchronize the access to the buffers, so we can use relaxed order
            auto front = m_front.fetch_xor(1, Ord::relaxed) ^ 1;    // emulate xor_fetch

            m_info.store(BufStatus::Idle, Ord::release);

            return { m_buffers[front], true };
        }

        bool update_buffers(std::invocable<Value&> auto&& update) noexcept
        {
            if (m_info.load(Ord::acquire) != BufStatus::Idle) {
                return false;
            }
            m_info.store(BufStatus::Updating, Ord::relaxed);

            auto back = m_front.load(Ord::relaxed) ^ 1;    // access the back buffer
            std::forward<decltype(update)>(update)(m_buffers[back]);

            m_info.store(BufStatus::Done, Ord::release);
            return true;
        }

        Value&       front() noexcept { return m_buffers[m_front.load(Ord::relaxed)]; }
        const Value& front() const noexcept { return m_buffers[m_front.load(Ord::relaxed)]; }

        Value&       back() noexcept { return m_buffers[m_front.load(Ord::relaxed) ^ 1]; }
        const Value& back() const noexcept { return m_buffers[m_front.load(Ord::relaxed) ^ 1]; }

        BufStatus status() const noexcept { return m_info.load(Ord::relaxed); }

    private:
        using Ord = std::memory_order;

        Buf                        m_buffers;
        std::atomic<BufStatus>     m_info  = BufStatus::Idle;
        std::atomic<std::uint32_t> m_front = 0;
    };

    // NOTE: if the following static_assert fails, the atomic operations are not lock-free, you may want to
    // disable this check and allow the code to compile, but be aware that the performance may be affected

#if not defined(DOUBLEBUF_DISABLE_LOCKFREE_CHECK)
    // lock-free requirements for atomic operations
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
#endif
}

#endif /* end of include guard: DOUBLEBUF_DOUBLEBUF_HPP_T4O73RT39J */
