#ifndef DOUBLE_BUFFER_ATOMIC_HPP_T4PFY34R
#define DOUBLE_BUFFER_ATOMIC_HPP_T4PFY34R

#include <array>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>

template <std::copyable Buffer, bool DynamicAlloc = false>
class DoubleBufferAtomic
{
public:
    using BufferType  = Buffer;
    using BuffersType = std::conditional_t<DynamicAlloc, std::unique_ptr<Buffer[]>, std::array<Buffer, 2>>;

    static bool constexpr s_dynamicAlloc = DynamicAlloc;

    enum class BufferUpdateStatus : std::uint16_t
    {
        Idle,
        Updating,
        Done,
    };

    explicit DoubleBufferAtomic(Buffer startState = {})
        requires(!DynamicAlloc)
        : m_buffers{ startState, startState }
    {
    }

    explicit DoubleBufferAtomic(Buffer startState = {})
        requires(DynamicAlloc)
        : m_buffers{ std::make_unique<Buffer[]>(2) }
    {
        m_buffers[0] = startState;
        m_buffers[1] = startState;
    }

    const Buffer& swapBuffers()
    {
        if (m_info != BufferUpdateStatus::Done) {
            return getFront();
        }

        // the index swapped here
        auto front = m_front.fetch_xor(1) ^ 1;    // emulate xor_fetch

        m_info = BufferUpdateStatus::Idle;

        return m_buffers[front];
    }

    void updateBuffer(std::invocable<Buffer&> auto&& update) noexcept
    {
        if (m_info != BufferUpdateStatus::Idle) {
            return;
        }
        m_info = BufferUpdateStatus::Updating;

        auto back = m_front ^ 1;    // access the back buffer
        std::forward<decltype(update)>(update)(m_buffers[back]);

        m_info = BufferUpdateStatus::Done;
    }

    const Buffer& getFront() const { return m_buffers[m_front]; }
    Buffer&       getFront() { return m_buffers[m_front]; }

    BufferUpdateStatus status() const { return m_info.load(); }

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

#endif /* end of include guard: DOUBLE_BUFFER_ATOMIC_HPP_T4PFY34R */
