#include <iostream>
#include <cmath>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <ranges>
#include <atomic>
#include <queue>

template <typename T>
class SharedArray {
public:
    SharedArray(size_t size) : size_(size) {
        ptr = mmap(
            /* addr= */ NULL,
            /* length= */ sizeof(T) * size,
            /* prot= */ PROT_READ | PROT_WRITE,
            /* flags= */ MAP_SHARED | MAP_ANONYMOUS,
            /* fd= */ -1,
            /* offset= */ 0);
        ptr_data_type = reinterpret_cast<T *>(ptr);
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("Something went wrong with map");
        }
    }

    ~SharedArray() {
        std::cout << "Destr" << std::endl;
        munmap(ptr, size_);
    }

    T* begin() {
        return ptr_data_type;
    }

    T* end() {
        return ptr_data_type + size_;
    }

    void set(size_t id, const T& value) {
        if (id >= size_) {
            throw std::runtime_error("Id is greater than array size");
        }

        ptr_data_type[id] = value;
    }

    T& operator[](size_t id) {
        return ptr_data_type[id];
    }

    size_t size() {
        return size_;
    }

protected:
    void *ptr;
    T *ptr_data_type;
    size_t size_;
};

template <typename T>
class SharedAtomicArray : public SharedArray<std::atomic<T>> {
public:
    SharedAtomicArray(size_t size) : SharedArray<std::atomic<T>>(size) {};
    SharedAtomicArray(size_t size, T desired) : SharedArray<std::atomic<T>>(size) {
        for (int i = 0; i < size; i++) {
            this->ptr_data_type[i] = desired;
        }
    }
    
};

template <typename R>
concept InputContiguousRange = std::ranges::contiguous_range<R>;

template <typename F, typename A>
concept StatelessFunction = std::invocable<F, A>; //?

template <typename R,  
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
void worker_private(SharedArray<B>& data_new, R& data_old, size_t work_size, size_t cur_size, size_t total_size, F f) {
    for (int i = cur_size; i < cur_size + work_size && i < total_size; i++) {
        data_new.set(i, f(data_old.data()[i]));
    }
}

template <typename R,  
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
void dynamic_worker_private(SharedArray<B>& data_new, R& data_old, F f, SharedAtomicArray<bool> &sync) {
    constexpr size_t M = 2;
    size_t current_index = 0;
    std::queue<size_t> q;
    while (current_index < data_new.size()) {
        while(q.size() < M && current_index < data_new.size()) {
            bool expected = false;
            sync[current_index].compare_exchange_strong(expected, true);
            if (!expected) {
                q.push(current_index);
            }
            current_index++;
        }

        while(!q.empty()) {
            size_t id = q.front();
            q.pop();
            data_new.set(id, f(data_old.data()[id]));
        }
    }
}

template <typename R,  
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
SharedArray<B> TransformWithProcesses(R&& range, F&& f, int nprocesses) {
    size_t range_size = range.size();
    size_t work_size = std::ceil((float) range_size / nprocesses);
    size_t cur_size = 0;

    SharedArray<B> arr(range_size);

    for (int i = 0; i < nprocesses; i++) {
        pid_t pid = fork();

        switch (pid) {
            case 0: {
                std::cout << "here" << std::endl;

                SharedAtomicArray<bool> sync(range_size, false);

                dynamic_worker_private(arr, range, f, sync);
                exit(EXIT_SUCCESS); // TODO ??
            }
            case -1:
                std::cerr << "Something went wrong" << std::endl;
                exit(EXIT_FAILURE); // TODO operate safier: kill all childs? continue?
            default:
                cur_size += work_size;
                break;
        }
    }
    pid_t wpid;
    int status = 0;
    while ((wpid = wait(&status)) > 0);

    return arr;
}

struct Point {
    int x, y;
};

bool is_in_sphere(Point p) {
    return p.x * p.x + p.y * p.y <= 1;
}

int main() {
    std::cout << std::atomic<bool>::is_always_lock_free << std::endl;
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto ans = TransformWithProcesses(data, [](int x) {return x * 3 + x * x * 2;}, 10);
    //std::cout << ans.size() << std::endl;
    for (auto e: ans) {
        std::cout << e << std::endl;
    }
}