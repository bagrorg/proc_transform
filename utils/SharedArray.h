#pragma once

#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>


template <typename T>
class SharedArray {
public:
    explicit SharedArray(size_t size) : size_(size) {
        ptr = mmap(NULL, sizeof(T) * size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("Something went wrong with map");
        }
        ptr_data_type = reinterpret_cast<T *>(ptr);
    }

    ~SharedArray() {
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
class SharedAtomicVariable : public SharedArray<T> {
public:
    SharedAtomicVariable() : SharedArray<T>(1) {
        sem_ptr = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); //todo make errno
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("Something went wrong with map");
        }
        sem = reinterpret_cast<sem_t*>(sem_ptr);   //is it good to make it one

        errno = 0;
        int ret = sem_init(sem, 1, 1);    //todo make errno
        if (ret < 0) {
            throw std::runtime_error("Something wrong with semaphore: " + std::string(strerror(errno)));
        }
    }

    ~SharedAtomicVariable() {
        munmap(sem_ptr, sizeof(sem_t));
    }
    
    bool cas(T &expected, T value) {
        bool res = false;
        sem_wait(sem);
        T cur = *(this->ptr_data_type);
        if (*(this->ptr_data_type) == expected) {
            res = true;
            *(this->ptr_data_type) = value;
        }
        expected = cur;
        sem_post(sem);
        return res;
    }

private:
    void *sem_ptr;
    sem_t *sem;
};