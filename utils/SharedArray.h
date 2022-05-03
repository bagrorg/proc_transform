#pragma once

#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>

template <typename T>
class SharedArray {
public:
    SharedArray(size_t size) : size_(size) {
        ptr = mmap(NULL, sizeof(T) * size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        ptr_data_type = reinterpret_cast<T *>(ptr);
        if (ptr == MAP_FAILED) {
            throw std::runtime_error("Something went wrong with map");
        }
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
class SharedAtomicArray : public SharedArray<T> { 
public:
    SharedAtomicArray(size_t size) : SharedArray<T>(size) {
        sem_ptr = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); //todo make errno
        sem = reinterpret_cast<sem_t*>(sem_ptr);   //is it good to make it one
        sem_init(sem, 1, 1);    //todo make errno
    }

    ~SharedAtomicArray() {
        munmap(sem_ptr, sizeof(sem_t));
    }
    
    bool atomicSet(size_t id, T value, T expected) {
        bool res = false;
        sem_wait(sem);
        if (this->ptr_data_type[id] == expected) {
            res = true;
            this->ptr_data_type[id] = value;
        }
        sem_post(sem);
        return res;
    }

private:
    void *sem_ptr;
    sem_t *sem;
};