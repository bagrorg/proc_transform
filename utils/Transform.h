#pragma once
#include "SharedArray.h"
#include <queue>
#include <unistd.h>
#include <cmath>
#include <ranges>
#include <iostream>
#include <wait.h>

enum TransformVersion { STAT, DYN };

template <TransformVersion V>
struct Tag {};

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
void worker_private(SharedArray<B>& data_new, R& data_old, size_t position, size_t work_size, F f) {
    for (size_t i = position; i < position + work_size && i < data_old.size(); i++) {
        data_new.set(i, f(data_old.data()[i]));
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
            data_new.set(i, f(data_old.data()[i]));
        }
    }
}

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

    for (int i = 0; i < nprocesses; i++) {
        pid_t pid = fork();

        switch (pid) {
            case 0: {
                worker_private(arr, range, position, work_size, f);
                exit(EXIT_SUCCESS); // TODO ??
            }
            case -1:
                std::cerr << "Something went wrong" << std::endl;
                exit(EXIT_FAILURE); // TODO operate safier: kill all childs? continue?
            default:
                position += work_size;
                break;
        }
    }
    pid_t wpid;
    int status = 0;
    while ((wpid = wait(&status)) > 0);

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

    for (int i = 0; i < nprocesses; i++) {
        pid_t pid = fork();

        switch (pid) {
            case 0: {
                dynamic_worker_private(arr, range, f, sync_offset, 8);
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