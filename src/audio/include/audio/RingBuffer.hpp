#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace audio {

// Single-producer / single-consumer lock-free ring buffer of PCM samples.
//
// This is the ONLY channel between the decode thread (producer) and the
// real-time audio callback (consumer). The callback contract (see ADR-0004)
// requires: no allocation, no locks, no I/O, no SQLite on the audio thread.
// All storage is allocated once in the constructor; push()/pop() only touch
// preallocated memory and two atomics.
//
// Usage: exactly one thread calls push() (producer) and exactly one thread
// calls pop() (consumer). Any other sharing is undefined.
template <typename T>
class RingBuffer {
public:
    // Rounds requested capacity up to a power of two (minimum 2) so index
    // wrapping is a cheap mask. Counters are monotonic, so the full capacity is
    // usable (no wasted slot).
    explicit RingBuffer(std::size_t requestedCapacity) {
        std::size_t cap = 2;
        while (cap < requestedCapacity) {
            cap <<= 1;
        }
        capacity_ = cap;
        mask_ = cap - 1;
        buffer_.resize(cap);
    }

    std::size_t capacity() const { return capacity_; }

    // Producer side.
    std::size_t availableWrite() const {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return capacity_ - (head - tail);
    }

    // Consumer side.
    std::size_t availableRead() const {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        return head - tail;
    }

    // Producer only. Writes up to n items; returns the number actually written
    // (less than n when the buffer is nearly full). Never blocks, never allocates.
    std::size_t push(const T* data, std::size_t n) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        const std::size_t space = capacity_ - (head - tail);
        const std::size_t toWrite = n < space ? n : space;
        for (std::size_t i = 0; i < toWrite; ++i) {
            buffer_[(head + i) & mask_] = data[i];
        }
        head_.store(head + toWrite, std::memory_order_release);
        return toWrite;
    }

    // Consumer only (real-time safe). Reads up to n items; returns the number
    // actually read. Never blocks, never allocates.
    std::size_t pop(T* out, std::size_t n) {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t avail = head - tail;
        const std::size_t toRead = n < avail ? n : avail;
        for (std::size_t i = 0; i < toRead; ++i) {
            out[i] = buffer_[(tail + i) & mask_];
        }
        tail_.store(tail + toRead, std::memory_order_release);
        return toRead;
    }

    // Consumer only. Drops all readable items without copying (e.g. on stop).
    void clear() {
        tail_.store(head_.load(std::memory_order_acquire),
                    std::memory_order_release);
    }

private:
    std::vector<T>           buffer_;
    std::size_t              capacity_ = 0;
    std::size_t              mask_ = 0;
    std::atomic<std::size_t> head_{0}; // written by producer
    std::atomic<std::size_t> tail_{0}; // written by consumer
};

} // namespace audio
