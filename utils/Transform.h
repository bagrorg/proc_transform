#pragma once
#include "SharedArray.h"
#include <queue>
#include <unistd.h>
#include <cmath>
#include <ranges>
#include <iostream>
#include <wait.h>
#include <cstring>

enum TransformVersion { STAT, DYN };

template <TransformVersion V>
struct Tag {};

template <typename R>
concept InputContiguousRange = std::ranges::contiguous_range<R>;

template <typename F, typename A>
concept StatelessFunction = std::invocable<F, A>;

template <typename R,
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
void worker_private(SharedArray<B>& data_new, R& data_old, size_t position, size_t work_size, F f) {
    for (size_t i = position; i < position + work_size && i < data_old.size(); i++) {
        data_new.set(i, f(*(data_old.begin() + i)));
    }
}

template <typename R,
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
void dynamic_worker_private(SharedArray<B>& data_new, R& data_old, F f, SharedAtomicVariable<size_t> &sync_offset, size_t M = 1) {
    size_t current_offset = 0;
    while (current_offset < data_old.size()) {
        while(!sync_offset.cas(current_offset, current_offset + M) && current_offset < data_old.size()) {}
        if (current_offset >= data_old.size()) break;

        for (size_t i = current_offset; i < current_offset + M && i < data_old.size(); i++) {
            data_new.set(i, f(*(data_old.begin() + i)));
        }
    }
}

void clearChilds(const std::vector<pid_t> &childs) {
    for (pid_t pid: childs) {
        errno = 0;
        int ret = kill(pid, SIGKILL);
        if (ret < 0) {
            if (errno != ESRCH) {
                std::cerr << "kill(): Unreachable code (" << strerror(errno) << ")" << std::endl;
            }
        }
    }
}

enum Mode { COMMON, EMERGENCE };
void waitForChilds(const std::vector<pid_t> &childs, Mode m = Mode::COMMON) {
    size_t processes_over = 0;
    int status;
    while (processes_over < childs.size()) {
        errno = 0;
        pid_t ret = waitpid(-1, &status, 0);
        if (ret < 0) {
            if (errno == ECHILD) {
                break;
            }

            if (m == Mode::EMERGENCE) {
                std::cerr << "waitpid(): Fatal error while waiting (" << strerror(errno) << ")" << std::endl;
            } else {
                clearChilds(childs);
                waitForChilds(childs, Mode::EMERGENCE);
                throw std::runtime_error("Something wrong while waiting: " + std::string(strerror(errno)));
            }
        }

        if (WIFEXITED(status)) {
            processes_over += 1;
        }
    }
}

struct FailedListener {
    SharedAtomicVariable<bool> failed = SharedAtomicVariable<bool>(false);

    void operator()(std::vector<pid_t> &childs) {
        bool expected = false;
        failed.cas(expected, false);
        if (expected) {
            clearChilds(childs);
            waitForChilds(childs, Mode::EMERGENCE);
            throw std::runtime_error("Error in worker process");
        }
    }
};

template <typename R,
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
SharedArray<B> TransformWithProcesses(R&& range, F&& f, size_t nprocesses, Tag<STAT>) {
    size_t range_size = range.size();
    size_t work_size = std::ceil((float) range_size / nprocesses);
    size_t position = 0;

    SharedArray<B> arr(range_size);
    std::vector<pid_t> childs;
    FailedListener isFailed;

    for (int i = 0; i < nprocesses; i++) {
        isFailed(childs);
        errno = 0;
        pid_t pid_f = fork();

        switch (pid_f) {
            case 0: {
                try {
                    worker_private(arr, range, position, work_size, f);
                } catch (std::exception &e) {
                    bool error = false;
                    
                    isFailed.failed.cas(error, true);
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            case -1:
                clearChilds(childs);
                waitForChilds(childs, Mode::EMERGENCE);
                throw std::runtime_error("Error with fork: " + std::string(strerror(errno)));
            default:
                childs.push_back(pid_f);
                position += work_size;
                break;
        }
    }
    
    waitForChilds(childs);
    isFailed(childs);
    return arr;
}

template <typename R,
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
SharedArray<B> TransformWithProcesses(R&& range, F&& f, size_t nprocesses, Tag<DYN>) {
    size_t range_size = range.size();
    size_t work_size = std::ceil((float) range_size / nprocesses);
    size_t cur_size = 0;

    SharedArray<B> arr(range_size);
    SharedAtomicVariable<size_t> sync_offset;
    std::vector<pid_t> childs;
    FailedListener isFailed;
    

    for (int i = 0; i < nprocesses; i++) {
        isFailed(childs);
        errno = 0;
        pid_t pid_f = fork();

        switch (pid_f) {
            case 0: {
                try {
                    dynamic_worker_private(arr, range, f, sync_offset, 8);
                } catch (std::exception &e) {
                    bool error = false;
                    isFailed.failed.cas(error, true);
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            case -1:
                clearChilds(childs);
                waitForChilds(childs, Mode::EMERGENCE);
                throw std::runtime_error("Error with fork: " + std::string(strerror(errno)));
            default:
                childs.push_back(pid_f);
                cur_size += work_size;
                break;
        }
    }

    waitForChilds(childs);
    isFailed(childs);

    return arr;
}

template <TransformVersion V,
          typename R,
          typename F,
          typename A = std::ranges::range_value_t<R>,
          typename B = typename std::invoke_result<F, A>::type,
          typename std::enable_if<std::is_trivially_copyable<B>::value, bool>::type = true>
requires (InputContiguousRange<R>, StatelessFunction<F, A>, std::ranges::range<SharedArray<B>>)
SharedArray<B> TransformWithProcesses(R&& range, F&& f, size_t nprocesses) {
    return TransformWithProcesses(range, f, nprocesses, Tag<V>{});
}