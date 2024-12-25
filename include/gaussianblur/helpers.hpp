
#pragma once
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
extern "C" {
#include "pffft_pommier/pffft.h"
}

#define L2_CACHE_SIZE (16 * 1024 * 1024)
#define MALLOC_V4SF_ALIGNMENT 64

static void *Valigned_malloc(size_t nb_bytes) {
  void *p, *p0 = malloc(nb_bytes + MALLOC_V4SF_ALIGNMENT);
  if (!p0) return (void *)0;
  p = (void *)(((size_t)p0 + MALLOC_V4SF_ALIGNMENT) &
               (~((size_t)(MALLOC_V4SF_ALIGNMENT - 1))));
  *((void **)p - 1) = p0;
  return p;
}

static void Valigned_free(void *p) {
  if (p) free(*((void **)p - 1));
}

template <class T>
class PFAlloc {
 public:
  // type definitions
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  // rebind allocator to type U
  template <class U>
  struct rebind {
    typedef PFAlloc<U> other;
  };

  // return address of values
  pointer address(reference value) const { return &value; }
  const_pointer address(const_reference value) const { return &value; }

  /* constructors and destructor
   * - nothing to do because the allocator has no state
   */
  PFAlloc() throw() {}
  PFAlloc(const PFAlloc &) throw() {}
  template <class U>
  PFAlloc(const PFAlloc<U> &) throw() {}
  ~PFAlloc() throw() {}

  // return maximum number of elements that can be allocated
  size_type max_size() const throw() {
    return std::numeric_limits<std::size_t>::max() / sizeof(T);
  }

  // allocate but don't initialize num elements of type T
  pointer allocate(size_type num, const void * = 0) {
    pointer ret = (pointer)Valigned_malloc(int(num) * sizeof(T));
    return ret;
  }

  // initialize elements of allocated storage p with value value
  void construct(pointer p, const T &value) {
    // initialize memory with placement new
    new ((void *)p) T(value);
  }

  // destroy elements of initialized storage p
  void destroy(pointer p) {
    // destroy objects by calling their destructor
    p->~T();
  }

  // deallocate storage p of deleted elements
  void deallocate(pointer p, size_type num) {
    // deallocate memory with pffft
    Valigned_free((void *)p);
  }
};
template <typename T>
using AlignedVector = typename std::vector<T, PFAlloc<T>>;
typedef std::vector<std::vector<float>> DeinterleavedChs;

typedef struct {
  int rows;
  int cols;
  int channels;
} ImgGeom;

typedef struct {
  std::vector<uint8_t> data;
  ImgGeom geom;
} Image;

typedef struct {
  int rows;
  int cols;
} TrailingZeros;

struct PFFFT_Deleter {
  void operator()(PFFFT_Setup *setup) const {
    if (setup) {
      pffft_destroy_setup(setup);
    }
  }
};

// Define unique_ptr types for PFFFT_Setup with custom deleter
using PFFFT_Setup_UniquePtr = std::unique_ptr<PFFFT_Setup, PFFFT_Deleter>;

typedef struct {
  AlignedVector<float> kerf_1D_row;
  AlignedVector<float> kerf_1D_col;
  PFFFT_Setup_UniquePtr rows_setup;
  PFFFT_Setup_UniquePtr cols_setup;
  int pad;
  TrailingZeros trailing_zeros;
} KernelDFT;

template <typename T, typename op>
void hybrid_loop(T end, op operation) {
  auto operation_wrapper = [&](T i, int tid = 0) {
    if constexpr (std::is_invocable_v<op, T>)
      operation(i);
    else
      operation(i, tid);
  };
#if defined(__EMSCRIPTEN_THREADS__) || defined(ENABLE_MULTITHREADING)
  const int num_threads = std::thread::hardware_concurrency();

  // Split in block equally for each thread. ex: 3 threads, start = 0, end = 8
  // Thread 0: 0,1,2
  // Thread 1: 3,4,5
  // Thread 2: 6,7
  // Also don't spawn more threads than needed
  // ex: 4 threads, start = 0, end = 3
  // Thread 0: 0
  // Thread 1: 1
  // Thread 2: 2
  // Thread 3: NOT SPAWNED
  const T block_size = (end + num_threads - 1) / num_threads;
  std::vector<std::thread> threads;
  const int threads_needed =
      std::min(num_threads, (int)std::ceil(end / (float)block_size));
  for (int tid = 0; tid < threads_needed; ++tid) {
    threads.emplace_back([=]() {
      T block_start = tid * block_size;
      T block_end =
          (tid == threads_needed - 1) ? end : block_start + block_size;

      for (T i = block_start; i < block_end; ++i) operation_wrapper(i, tid);
    });
  }
  for (auto &thread : threads) thread.join();
#else
  for (T i = 0; i < end; ++i) operation_wrapper(i);
#endif
}

//!
//! \brief This function performs a 2D tranposition of an image.
//!
//! The transposition is done per
//! block to reduce the number of cache misses and improve cache coherency for
//! large image buffers. Templated by buffer data type T and buffer number of
//! channels C.
//!
//! \param[in] in           source buffer
//! \param[in,out] out      target buffer
//! \param[in] w            image width
//! \param[in] h            image height
//!
template <int C, typename T>
void flip_block(const T *in, T *out, const int w, const int h) {
  // Suppose a square block of L2 cache size = 16MB
  // to be divided for the num of channels and bytes
  const int block = sqrt((double)L2_CACHE_SIZE / (C * sizeof(T)));
  const int w_blocks = std::ceil((float)w / block);
  const int h_blocks = std::ceil((float)h / block);
  const int last_blockx = w % block == 0 ? block : w % block;
  const int last_blocky = h % block == 0 ? block : h % block;

  hybrid_loop(w_blocks * h_blocks, [&](int n) {
    const int x = n / h_blocks;
    const int y = n % h_blocks;
    const int blockx = (x == w_blocks - 1) ? last_blockx : block;
    const int blocky = (y == h_blocks - 1) ? last_blocky : block;

    const T *p = in + block * (y * w * C + x * C);
    T *q = out + block * (y * C + x * h * C);

    for (int xx = 0; xx < blockx; xx++) {
      for (int yy = 0; yy < blocky; yy++) {
        for (int k = 0; k < C; k++) q[k] = p[k];
        p += w * C;
        q += C;
      }
      p += C * (1 - blocky * w);
      q += C * (h - blocky);
    }
  });
}

template <uint32_t Channels, typename T, typename U>
void deinterleave_channels(const T *const interleaved, U **const deinterleaved,
                           const uint32_t total_size) {
  // Cache-friendly deinterleave, splitting for blocks of 16 MB, inspired by
  // flip-block
  constexpr float round =
      std::is_integral_v<U> ? std::is_integral_v<T> ? 0 : 0.5F : 0;
  constexpr uint32_t block =
      L2_CACHE_SIZE / (Channels * std::max(sizeof(T), sizeof(U)));
  const uint32_t num_blocks = std::ceil(total_size / (float)block);
  const uint32_t last_block_size =
      total_size % block == 0 ? block : total_size % block;

  hybrid_loop(num_blocks, [&](auto n) {
    const uint32_t x = n * block;
    U *channel_ptrs[Channels];
    for (uint32_t c = 0; c < Channels; ++c) {
      channel_ptrs[c] = deinterleaved[c] + x;
    }
    const T *const interleaved_ptr = interleaved + (x * Channels);

    const int blockx = (n == num_blocks - 1) ? last_block_size : block;
    for (int xx = 0; xx < blockx; ++xx) {
      for (uint32_t c = 0; c < Channels; ++c) {
        channel_ptrs[c][xx] = interleaved_ptr[(xx * Channels) + c] + round;
      }
    }
  });
}

template <uint32_t Channels, typename T, typename U>
void interleave_channels(const U **const deinterleaved, T *const interleaved,
                         const uint32_t total_size) {
  constexpr float round =
      std::is_integral_v<T> ? std::is_integral_v<U> ? 0 : 0.5F : 0;
  constexpr uint32_t block =
      L2_CACHE_SIZE / (Channels * std::max(sizeof(T), sizeof(U)));
  const uint32_t num_blocks = std::ceil(total_size / (float)block);
  const uint32_t last_block_size =
      total_size % block == 0 ? block : total_size % block;

  hybrid_loop(num_blocks, [&](auto n) {
    const uint32_t x = n * block;
    const U *channel_ptrs[Channels];
    for (uint32_t c = 0; c < Channels; ++c) {
      channel_ptrs[c] = deinterleaved[c] + x;
    }
    T *const interleaved_ptr = interleaved + x * Channels;

    const int blockx = (n == num_blocks - 1) ? last_block_size : block;
    for (int xx = 0; xx < blockx; ++xx) {
      for (uint32_t c = 0; c < Channels; ++c) {
        interleaved_ptr[xx * Channels + c] = channel_ptrs[c][xx] + round;
      }
    }
  });
}