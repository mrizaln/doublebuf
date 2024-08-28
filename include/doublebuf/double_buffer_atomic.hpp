#ifndef DOUBLE_BUFFER_ATOMIC_HPP_T4PFY34R
#define DOUBLE_BUFFER_ATOMIC_HPP_T4PFY34R

#include <array>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace doublebuf
{
    template <std::movable Buffer, bool DynamicAlloc = false>
    class DoubleBufferAtomic
    {
    public:
        using BufferType = Buffer;
        using BuffersType
            = std::conditional_t<DynamicAlloc, std::unique_ptr<Buffer[]>, std::array<Buffer, 2>>;

        static bool constexpr s_dynamicAlloc = DynamicAlloc;

        enum class BufferUpdateStatus : std::uint16_t
        {
            Idle,
            Updating,
            Done,
        };

        struct [[nodiscard]] SwapResult
        {
            BufferType& m_buffer;
            bool        m_swapped;
        };

        explicit DoubleBufferAtomic(Buffer front = {}, Buffer back = {})
            requires(!DynamicAlloc)
            : m_buffers{ std::move(front), std::move(back) }
        {
        }

        explicit DoubleBufferAtomic(Buffer front = {}, Buffer back = {})
            requires(DynamicAlloc)
            : m_buffers{ std::make_unique<Buffer[]>(2) }
        {
            m_buffers[0] = std::move(front);
            m_buffers[1] = std::move(back);
        }

        SwapResult swapBuffers() noexcept
        {
            if (m_info != BufferUpdateStatus::Done) {
                return { m_buffers[m_front], false };
            }

            // the index swapped here
            auto front = m_front.fetch_xor(1) ^ 1;    // emulate xor_fetch

            m_info = BufferUpdateStatus::Idle;

            return { m_buffers[front], true };
        }

        bool updateBuffers(std::invocable<Buffer&> auto&& update) noexcept
        {
            if (m_info != BufferUpdateStatus::Idle) {
                return false;
            }
            m_info = BufferUpdateStatus::Updating;

            auto back = m_front ^ 1;    // access the back buffer
            std::forward<decltype(update)>(update)(m_buffers[back]);

            m_info = BufferUpdateStatus::Done;
            return true;
        }

        Buffer&       front() noexcept { return m_buffers[m_front]; }
        const Buffer& front() const noexcept { return m_buffers[m_front]; }

        Buffer&       back() noexcept { return m_buffers[m_front ^ 1]; }
        const Buffer& back() const noexcept { return m_buffers[m_front ^ 1]; }

        BufferUpdateStatus status() const noexcept { return m_info; }

    private:
        enum class BufferIndexCombined : std::uint8_t
        {
            FirstSecond = 0,
            SecondFirst = 1,
        };

        BuffersType                     m_buffers;
        std::atomic<BufferUpdateStatus> m_info  = BufferUpdateStatus::Idle;
        std::atomic<std::size_t>        m_front = 0;
    };
}

#endif /* end of include guard: DOUBLE_BUFFER_ATOMIC_HPP_T4PFY34R */
