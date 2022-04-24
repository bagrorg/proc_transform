#include <iostream>
#include <cmath>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
//template <InputContiguousRange R, StatelessFunction F>
//    requires ReturnTypeIsTriviallyCopyable<F>

template <typename T, typename F>
void worker_private(T *data_new, T *data_old, size_t work_size, size_t cur_size, size_t total_size, F f) {
    for (int i = cur_size; i < cur_size + work_size && i < total_size; i++) {
        data_new[i] = f(data_old[i]);
    }
}

template <typename F>
auto TransformWithProcesses(std::vector<int> range, F f, int nprocesses) /* -> TransformedContiguousRange*/  {
    size_t range_size = range.size();
    size_t work_size = std::ceil((float) range_size / nprocesses);
    size_t cur_size = 0;

    void *shared = mmap(
            /* addr= */ NULL,
            /* length= */ range_size,
            /* prot= */ PROT_READ | PROT_WRITE,
            /* flags= */ MAP_SHARED | MAP_ANONYMOUS,
            /* fd= */ -1,
            /* offset= */ 0);
    if (shared == MAP_FAILED) {
        std::cerr << "Something went wrong with map" << std::endl;
        return;
    }

    for (int i = 0; i < nprocesses; i++) {
        pid_t pid = fork();

        switch (pid) {
            case 0:
                worker_private((int *) shared, range.data(), work_size, cur_size, range_size, f);
                munmap(shared, range_size);
                sleep(i);
                std::cout << "END FROM CHILD" << std::endl;
                exit(EXIT_SUCCESS); // TODO ??
            case -1:
                std::cerr << "Something went wrong" << std::endl;
                exit(EXIT_FAILURE); // TODO operate safier
            default:
                cur_size += work_size;
                break;
        }
    }
    pid_t wpid;
    int status = 0;
    while ((wpid = wait(&status)) > 0);

    std::cout << "Hello from parent" << std::endl;
    for (int i = 0; i < range_size; i++) {
        std::cout << ((int *) shared)[i] << std::endl;
    }
    munmap(shared, range_size);

}

int main() {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    TransformWithProcesses(data, [](int x) {return x * x;}, 3);
}