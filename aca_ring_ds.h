#ifndef ACA_RING_DS_H
#define ACA_RING_DS_H

#include <stddef.h>

#ifdef __cplusplus
#include <atomic>
#include <cstddef>

#define ACA_RING_ATOMIC(type) std::atomic<type>
#define ACA_RING_MEMORY_ORDER(order) std::order
#define ACA_RING_ATOMIC_LOAD(ptr, order) ((ptr)->load(ACA_RING_MEMORY_ORDER(order)))
#define ACA_RING_ATOMIC_STORE(ptr, value, order)                                                   \
    ((ptr)->store((value), ACA_RING_MEMORY_ORDER(order)))
#else
#include <stdatomic.h>
#define ACA_RING_ATOMIC(type) _Atomic(type)
#define ACA_RING_MEMORY_ORDER(order) order
#define ACA_RING_ATOMIC_LOAD(ptr, order) atomic_load_explicit((ptr), (order))
#define ACA_RING_ATOMIC_STORE(ptr, value, order) atomic_store_explicit((ptr), (value), (order))
#endif

typedef enum aca_ring_buffer_ds_type {
    ACA_RING_BUFFER_DS = 0,
    ACA_RING_BUFFER_POW2_DS,
} aca_ring_buffer_ds_type_t;

typedef enum aca_ring_queue_ds_type {
    ACA_RING_QUEUE_OVERWRITE_DS,
    ACA_RING_QUEUE_OVERWRITE_POW2_DS,
    ACA_RING_QUEUE_REJECT_DS,
    ACA_RING_QUEUE_REJECT_POW2_DS,
    ACA_RING_QUEUE_ASSERT_DS,
    ACA_RING_QUEUE_ASSERT_POW2_DS,
} aca_ring_queue_ds_type_t;

typedef enum aca_ring_queue_spsc_ds_type {
    ACA_RING_QUEUE_SPSC_REJECT_DS,
    ACA_RING_QUEUE_SPSC_REJECT_POW2_DS,
    ACA_RING_QUEUE_SPSC_ASSERT_DS,
    ACA_RING_QUEUE_SPSC_ASSERT_POW2_DS,
} aca_ring_queue_spsc_ds_type_t;

typedef enum aca_ring_queue_ds_full_behavior {
    ACA_RING_QUEUE_OVERWRITE,
    ACA_RING_QUEUE_REJECT,
    ACA_RING_QUEUE_ASSERT,
} aca_ring_queue_ds_full_behavior_t;

typedef struct aca_ring_queue_ds_config {
    size_t                            capacity;
    int                               isHeapAlloced;
    aca_ring_queue_ds_full_behavior_t fullBehavior;
} aca_ring_queue_config_t;

typedef struct aca_ring_buffer_ds_header {
    size_t                    size;
    size_t                    head;
    aca_ring_buffer_ds_type_t type;
} aca_ring_buffer_ds_header_t;

typedef struct aca_ring_buffer_spsc_ds_header {
    size_t size;
    ACA_RING_ATOMIC(size_t) head; // owned by the consumer
} aca_ring_buffer_spsc_ds_header_t;

typedef struct aca_ring_queue_ds_header {
    size_t                   capacity;
    size_t                   elemSize;
    size_t                   head;
    size_t                   tail;
    aca_ring_queue_ds_type_t type;
    int                      isHeapAlloced;
} aca_ring_queue_ds_header_t;

typedef struct aca_ring_queue_spsc_ds_header {
    size_t                        capacity;
    size_t                        elemSize;
    aca_ring_queue_spsc_ds_type_t type;
    int                           isHeapAlloced;
    ACA_RING_ATOMIC(size_t) head; // owned by the consumer
    ACA_RING_ATOMIC(size_t) tail; // owned by the producer
} aca_ring_queue_spsc_ds_header_t;

#define ACA_RING_BUFFER_RESERVE(elemSize, count)                                                   \
    ((count) * (elemSize) + sizeof(aca_ring_buffer_ds_header_t))
#define ACA_RING_BUFFER_RESERVE_FOR(T, count) ACA_RING_BUFFER_RESERVE(sizeof(T), (count))
#define ACA_RING_BUFFER_SPSC_RESERVE(elemSize, count)                                              \
    ((count) * (elemSize) + sizeof(aca_ring_buffer_spsc_ds_header_t))
#define ACA_RING_BUFFER_SPSC_RESERVE_FOR(T, count) ACA_RING_BUFFER_SPSC_RESERVE(sizeof(T), (count))
#define ACA_RING_QUEUE_RESERVE(elemSize, count)                                                    \
    ((count) * (sizeof(elemSize)) + sizeof(aca_ring_queue_ds_header_t))
#define ACA_RING_QUEUE_RESERVE_FOR(T, count) ACA_RING_QUEUE_RESERVE(sizeof(T), (count))
#define ACA_RING_QUEUE_SPSC_RESERVE(elemSize, count)                                               \
    ((count) * (elemSize) + sizeof(aca_ring_queue_spsc_ds_header_t))
#define ACA_RING_QUEUE_SPSC_RESERVE_FOR(T, count) ACA_RING_QUEUE_SPSC_RESERVE(sizeof(T), (count))

// ------------------------------------------------------------------------------------------------
// acaRingBuffer API
#ifdef __cplusplus
extern "C" {
#endif
void  *acaRingBufferCreateImpl(void *buffer, size_t elemSize, size_t capacity);
void   acaRingBufferFree(void *buffer);
size_t acaRingBufferCapacity(void *buffer);
size_t acaRingBufferFront(void *buffer);
void   acaRingBufferNext(void *buffer);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
template <typename T>
static T *acaRingBufferCreateCpp(T *buffer, size_t elemSize, size_t capacity) {
    return (T *)acaRingBufferCreateImpl(buffer, elemSize, capacity);
}
#define acaRingBufferCreate(T, size) /*NOLINT(readability-identifier-naming)*/                     \
    ((T) = acaRingBufferCreateCpp((T), (sizeof(*(T))), (size)))
#else
#define acaRingBufferCreate(T, size) /*NOLINT(readability-identifier-naming)*/                     \
    (T) = (acaRingBufferCreateImpl((T), (sizeof(*(T))), (size)))
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
// concurrent acaRingBuffer API - lock-free, single producer / single consumer (spsc)
#ifdef __cplusplus
extern "C" {
#endif
void  *acaRingBufferCreateSpscImpl(void *buffer, size_t elemSize, size_t capacity);
void   acaRingBufferSpscFree(void *buffer);
size_t acaRingBufferSpscCapacity(void *buffer);
size_t acaRingBufferSpscFront(void *buffer);
void   acaRingBufferSpscNext(void *buffer);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
template <typename T>
static T *acaRingBufferCreateSpscCpp(T *buffer, size_t elemSize, size_t capacity) {
    return (T *)acaRingBufferCreateSpscImpl(buffer, elemSize, capacity);
}
#define acaRingBufferSpscCreate(T, size) /*NOLINT(readability-identifier-naming)*/                 \
    ((T) = acaRingBufferCreateSpscCpp((T), (sizeof(*(T))), (size)))
#else
#define acaRingBufferSpscCreate(T, size) /*NOLINT(readability-identifier-naming)*/                 \
    (T) = (acaRingBufferCreateSpscImpl((T), (sizeof(*(T))), (size)))
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
// acaRingQueue API
#ifdef __cplusplus
extern "C" {
#endif
void  *acaRingQueueCreateImpl(void *queue, size_t elemSize, const aca_ring_queue_config_t *config);
void   acaRingQueueFree(void *queue);
size_t acaRingQueueSize(void *queue);
size_t acaRingQueueCapacity(void *queue);
int    acaRingQueueEnqueue(void *queue, const void *elem);
size_t acaRingQueueDequeue(void *queue);
size_t acaRingQueueFront(void *queue);
int    acaRingQueueEmpty(void *queue);
int    acaRingQueueFull(void *queue);
void  *acaRingQueueResize(void *oldQueue, size_t newSize);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
template <typename T>
static T *acaRingQueueCreateCpp(T *queue, size_t elemSize, const aca_ring_queue_config_t *config) {
    return (T *)acaRingQueueCreateImpl(queue, elemSize, config);
}
#define acaRingQueueCreate(T, config) /*NOLINT(readability-identifier-naming)*/                    \
    ((T) = acaRingQueueCreateCpp((T), (sizeof(*(T))), (config)))
#else
#define acaRingQueueCreate(T, config) /*NOLINT(readability-identifier-naming)*/                    \
    (T) = (acaRingQueueCreateImpl((T), (sizeof(*(T))), (config)))
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
// concurrent acaRingQueue API - lock-free, single producer / single consumer (spsc)
#ifdef __cplusplus
extern "C" {
#endif
void *
acaRingQueueCreateSpscImpl(void *queue, size_t elemSize, const aca_ring_queue_config_t *config);
void   acaRingQueueSpscFree(void *queue);
size_t acaRingQueueSpscSize(void *queue);
size_t acaRingQueueSpscCapacity(void *queue);
int    acaRingQueueSpscEnqueue(void *queue, const void *elem);
size_t acaRingQueueSpscDequeue(void *queue);
size_t acaRingQueueSpscFront(void *queue);
int    acaRingQueueSpscEmpty(void *queue);
int    acaRingQueueSpscFull(void *queue);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
template <typename T>
static T *
acaRingQueueCreateSpscCpp(T *queue, size_t elemSize, const aca_ring_queue_config_t *config) {
    return (T *)acaRingQueueCreateSpscImpl(queue, elemSize, config);
}
#define acaRingQueueSpscCreate(T, config) /*NOLINT(readability-identifier-naming)*/                \
    ((T) = acaRingQueueCreateSpscCpp((T), (sizeof(*(T))), (config)))
#else
#define acaRingQueueSpscCreate(T, config) /*NOLINT(readability-identifier-naming)*/                \
    (T) = (acaRingQueueCreateSpscImpl((T), (sizeof(*(T))), (config)))
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
#ifdef ACA_RING_DS_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline aca_ring_buffer_ds_header_t *getRingBufferHeader(void *buffer) {
    return ((aca_ring_buffer_ds_header_t *)buffer) - 1;
}

static inline int isPow2(size_t x) {
    return (x > 0) && ((x & (x - 1)) == 0);
}

static inline aca_ring_queue_ds_header_t *getRingQueueHeader(void *queue) {
    return ((aca_ring_queue_ds_header_t *)queue) - 1;
}

static inline size_t findNextRingQueueIndex(aca_ring_queue_ds_header_t *header, size_t index) {
    switch (header->type) {
        case ACA_RING_QUEUE_ASSERT_POW2_DS:
        case ACA_RING_QUEUE_REJECT_POW2_DS:
        case ACA_RING_QUEUE_OVERWRITE_POW2_DS:
            index = (index + 1) & (header->capacity - 1);
            break;
        default:
            index = (index + 1) % header->capacity;
            break;
    }
    return index;
}

static inline aca_ring_queue_ds_header_t *reallocRingQueue(void *queue, size_t newCapacity) {
    aca_ring_queue_ds_header_t *pOldHeader  = getRingQueueHeader(queue);
    size_t                      currentSize = acaRingQueueSize(queue);
    if (newCapacity < currentSize || !pOldHeader->isHeapAlloced) {
        return NULL;
    }

    size_t newSize = (pOldHeader->elemSize * newCapacity) + sizeof(aca_ring_queue_ds_header_t);
    aca_ring_queue_ds_header_t *pNewHeader = (aca_ring_queue_ds_header_t *)malloc(newSize);
    if (pNewHeader == NULL) {
        assert(0 && "failed to allocate memory for ring queue!"); // rare, so scream if it happens
        return NULL;
    }

    char *pOldData = (char *)(pOldHeader + 1);
    char *pNewData = (char *)(pNewHeader + 1);
    if (pOldHeader->head < pOldHeader->tail) {
        // not wrapped around, can copy in one go
        memcpy(pNewData,
               pOldData + (pOldHeader->head * pOldHeader->elemSize),
               currentSize * pOldHeader->elemSize);
    } else {
        // wrapped around, need to copy in two chunks
        size_t firstChunk  = pOldHeader->capacity - pOldHeader->head;
        size_t secondChunk = pOldHeader->tail;
        memcpy(pNewData,
               pOldData + (pOldHeader->head * pOldHeader->elemSize),
               firstChunk * pOldHeader->elemSize);
        memcpy(pNewData + (firstChunk * pOldHeader->elemSize),
               pOldData,
               secondChunk * pOldHeader->elemSize);
    }
    pNewHeader->capacity = newCapacity;
    pNewHeader->elemSize = pOldHeader->elemSize;
    pNewHeader->head     = 0;
    pNewHeader->tail     = acaRingQueueSize(queue);
    pNewHeader->type     = pOldHeader->type;
    free(pOldHeader);

    return pNewHeader;
}

void *acaRingBufferCreateImpl(void *buffer, size_t elemSize, size_t capacity) {
    aca_ring_buffer_ds_header_t *pHeader = nullptr;
    if (buffer == NULL) {
        pHeader =
            (aca_ring_buffer_ds_header_t *)malloc(ACA_RING_BUFFER_RESERVE(elemSize, capacity));
        if (pHeader == NULL) {
            return NULL;
        }
    } else {
        pHeader = (aca_ring_buffer_ds_header_t *)buffer;
    }
    pHeader->size = capacity;
    pHeader->head = 0;

    if ((capacity > 0) && ((capacity & (capacity - 1)) == 0)) {
        pHeader->type = ACA_RING_BUFFER_POW2_DS;
    } else {
        pHeader->type = ACA_RING_BUFFER_DS;
    }

    return (pHeader + 1); // return pointer to data, not header
}

void acaRingBufferFree(void *buffer) {
    if (buffer == NULL) {
        return;
    }
    free(getRingBufferHeader(buffer));
}

size_t acaRingBufferCapacity(void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    return getRingBufferHeader(buffer)->size;
}

size_t acaRingBufferFront(void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    aca_ring_buffer_ds_header_t *pHeader = getRingBufferHeader(buffer);
    return pHeader->head;
}

void acaRingBufferNext(void *buffer) {
    if (buffer == NULL) {
        return;
    }
    aca_ring_buffer_ds_header_t *pHeader = getRingBufferHeader(buffer);
    switch (pHeader->type) {
        case ACA_RING_BUFFER_POW2_DS:
            pHeader->head = (pHeader->head + 1) & (pHeader->size - 1);
            break;
        case ACA_RING_BUFFER_DS:
            pHeader->head = (pHeader->head + 1) % (pHeader->size);
            break;
        default:
            assert(0 && "invalid ring buffer type!");
            break;
    }
}

void *acaRingQueueCreateImpl(void *queue, size_t elemSize, const aca_ring_queue_config_t *config) {
    if (config == NULL || config->capacity == 0 || elemSize == 0) {
        return NULL;
    }
    aca_ring_queue_ds_header_t *pHeader = nullptr;
    if (queue == NULL) {
        pHeader = (aca_ring_queue_ds_header_t *)malloc(
            ACA_RING_QUEUE_RESERVE(elemSize, config->capacity));
        if (pHeader == NULL) {
            return NULL;
        }
        pHeader->isHeapAlloced = 1;
    } else {
        pHeader                = (aca_ring_queue_ds_header_t *)queue;
        pHeader->isHeapAlloced = config->isHeapAlloced;
    }
    pHeader->capacity = config->capacity;
    pHeader->elemSize = elemSize;
    pHeader->head     = 0;
    pHeader->tail     = 0;

    const int isCapacityPow2 = isPow2(config->capacity);
    if (isCapacityPow2) {
        switch (config->fullBehavior) {
            case ACA_RING_QUEUE_OVERWRITE:
                pHeader->type = ACA_RING_QUEUE_OVERWRITE_POW2_DS;
                break;
            case ACA_RING_QUEUE_REJECT:
                pHeader->type = ACA_RING_QUEUE_REJECT_POW2_DS;
                break;
            case ACA_RING_QUEUE_ASSERT:
                pHeader->type = ACA_RING_QUEUE_ASSERT_POW2_DS;
                break;
            default:
                assert(0 && "unknown full behavior!");
                break;
        }
    } else {
        switch (config->fullBehavior) {
            case ACA_RING_QUEUE_OVERWRITE:
                pHeader->type = ACA_RING_QUEUE_OVERWRITE_DS;
                break;
            case ACA_RING_QUEUE_REJECT:
                pHeader->type = ACA_RING_QUEUE_REJECT_DS;
                break;
            case ACA_RING_QUEUE_ASSERT:
                pHeader->type = ACA_RING_QUEUE_ASSERT_DS;
                break;
            default:
                assert(0 && "unknown full behavior!");
                break;
        }
    }

    return (pHeader + 1); // return pointer to data, not header
}

void acaRingQueueFree(void *queue) {
    if (queue == NULL) {
        return;
    }
    free(getRingQueueHeader(queue));
}

size_t acaRingQueueSize(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    aca_ring_queue_ds_header_t *pHeader = getRingQueueHeader(queue);
    if (pHeader->tail >= pHeader->head) {
        return pHeader->tail - pHeader->head;
    } else {
        return pHeader->capacity - (pHeader->head - pHeader->tail);
    }
}

size_t acaRingQueueCapacity(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    return getRingQueueHeader(queue)->capacity;
}

int acaRingQueueEnqueue(void *queue, const void *elem) {
    if (queue == NULL || elem == NULL) {
        return 0;
    }
    aca_ring_queue_ds_header_t *pHeader = getRingQueueHeader(queue);
    if (acaRingQueueFull(queue)) {
        switch (pHeader->type) {
            case ACA_RING_QUEUE_OVERWRITE_DS:
            case ACA_RING_QUEUE_OVERWRITE_POW2_DS:
                // overwrite the oldest element
                pHeader->head = findNextRingQueueIndex(pHeader, pHeader->head);
                break;
            case ACA_RING_QUEUE_REJECT_DS:
            case ACA_RING_QUEUE_REJECT_POW2_DS:
                // reject new element, do nothing
                return 0;
            case ACA_RING_QUEUE_ASSERT_DS:
            case ACA_RING_QUEUE_ASSERT_POW2_DS:
                // assert failure
                assert(0 && "ring queue is full!");
                return 0; // non-debug builds this falls back to "reject" behavior
            default:
                assert(0 && "unreachable");
                return 0;
        }
    }

    char  *pDataPtr = (char *)(pHeader + 1);
    size_t offset   = pHeader->tail * pHeader->elemSize;
    memcpy(pDataPtr + offset, elem, pHeader->elemSize);

    pHeader->tail = findNextRingQueueIndex(pHeader, pHeader->tail);
    return 1; // success
}

size_t acaRingQueueDequeue(void *queue) {
    if (queue == NULL) {
        return 0;
    }

    aca_ring_queue_ds_header_t *pHeader = getRingQueueHeader(queue);
    if (acaRingQueueEmpty(queue)) {
        return 0; // queue is empty
    }

    size_t frontIndex = pHeader->head;
    pHeader->head     = findNextRingQueueIndex(pHeader, pHeader->head);

    return frontIndex;
}

size_t acaRingQueueFront(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    return getRingQueueHeader(queue)->head;
}

int acaRingQueueEmpty(void *queue) {
    if (queue == NULL) {
        return 1; // consider NULL queue as empty
    }
    aca_ring_queue_ds_header_t *pHeader = getRingQueueHeader(queue);
    return pHeader->head == pHeader->tail;
}

int acaRingQueueFull(void *queue) {
    if (queue == NULL) {
        return 0; // consider NULL queue as not full
    }
    aca_ring_queue_ds_header_t *pHeader = getRingQueueHeader(queue);
    return findNextRingQueueIndex(pHeader, pHeader->tail) == pHeader->head;
}

void *acaRingQueueResize(void *oldQueue, size_t newCapacity) {
    if (oldQueue == NULL) {
        return NULL;
    }
    aca_ring_queue_ds_header_t *pNewHeader = reallocRingQueue(oldQueue, newCapacity);
    if (pNewHeader == NULL) {
        return NULL;
    }
    return (pNewHeader + 1); // return pointer to data, not header
}

// acaRingBufferLf implementation
static inline aca_ring_buffer_spsc_ds_header_t *getRingBufferLfHeader(void *buffer) {
    return ((aca_ring_buffer_spsc_ds_header_t *)buffer) - 1;
}

void *acaRingBufferCreateSpscImpl(void *buffer, size_t elemSize, size_t capacity) {
    if (elemSize == 0 || capacity == 0) {
        return NULL;
    }
    aca_ring_buffer_spsc_ds_header_t *pHeader = nullptr;
    if (buffer == NULL) {
        pHeader = (aca_ring_buffer_spsc_ds_header_t *)malloc(
            ACA_RING_BUFFER_SPSC_RESERVE(elemSize, capacity));
        if (pHeader == NULL) {
            return NULL;
        }
    } else {
        pHeader = (aca_ring_buffer_spsc_ds_header_t *)buffer;
    }
    pHeader->size = capacity;
    ACA_RING_ATOMIC_STORE(&pHeader->head, 0, memory_order_relaxed);

    return (void *)(pHeader + 1); // return pointer to data, not header
}

void acaRingBufferSpscFree(void *buffer) {
    if (buffer == NULL) {
        return;
    }
    free(getRingBufferLfHeader(buffer));
}

size_t acaRingBufferSpscCapacity(void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    return getRingBufferLfHeader(buffer)->size;
}

size_t acaRingBufferSpscFront(void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    aca_ring_buffer_spsc_ds_header_t *pHeader = getRingBufferLfHeader(buffer);
    return ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_acquire);
}

void acaRingBufferSpscNext(void *buffer) {
    if (buffer == NULL) {
        return;
    }
    aca_ring_buffer_spsc_ds_header_t *pHeader = getRingBufferLfHeader(buffer);
    size_t head = ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_relaxed);
    ACA_RING_ATOMIC_STORE(&pHeader->head, (head + 1) & (pHeader->size - 1), memory_order_release);
}

// acaRingQueueLf implementation
static inline aca_ring_queue_spsc_ds_header_t *getRingQueueLfHeader(void *queue) {
    return ((aca_ring_queue_spsc_ds_header_t *)queue) - 1;
}

void *
acaRingQueueCreateSpscImpl(void *queue, size_t elemSize, const aca_ring_queue_config_t *config) {
    if (config == NULL || elemSize == 0 || config->capacity == 0) {
        return NULL;
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = nullptr;
    if (queue == NULL) {
        pHeader = (aca_ring_queue_spsc_ds_header_t *)malloc(
            ACA_RING_QUEUE_SPSC_RESERVE(elemSize, config->capacity));
        if (pHeader == NULL) {
            return NULL;
        }
        pHeader->isHeapAlloced = 1;
    } else {
        pHeader                = (aca_ring_queue_spsc_ds_header_t *)queue;
        pHeader->isHeapAlloced = config->isHeapAlloced;
    }
    pHeader->capacity = config->capacity;
    pHeader->elemSize = elemSize;
    ACA_RING_ATOMIC_STORE(&pHeader->head, 0, memory_order_relaxed);
    ACA_RING_ATOMIC_STORE(&pHeader->tail, 0, memory_order_relaxed);

    const int isCapacityPow2 = isPow2(config->capacity);
    if (isCapacityPow2) {
        switch (config->fullBehavior) {
            case ACA_RING_QUEUE_REJECT:
                pHeader->type = ACA_RING_QUEUE_SPSC_REJECT_POW2_DS;
                break;
            case ACA_RING_QUEUE_ASSERT:
                pHeader->type = ACA_RING_QUEUE_SPSC_ASSERT_POW2_DS;
                break;
            default:
                assert(0 && "unknown full behavior!");
                break;
        }
    } else {
        switch (config->fullBehavior) {
            case ACA_RING_QUEUE_REJECT:
                pHeader->type = ACA_RING_QUEUE_SPSC_REJECT_DS;
                break;
            case ACA_RING_QUEUE_ASSERT:
                pHeader->type = ACA_RING_QUEUE_SPSC_ASSERT_DS;
                break;
            default:
                assert(0 && "unknown full behavior!");
                break;
        }
    }

    return (void *)(pHeader + 1); // return pointer to data, not header
}

void acaRingQueueSpscFree(void *queue) {
    if (queue == NULL) {
        return;
    }
    free(getRingQueueLfHeader(queue));
}

size_t acaRingQueueSpscSize(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = getRingQueueLfHeader(queue);
    size_t head = ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_acquire);
    size_t tail = ACA_RING_ATOMIC_LOAD(&pHeader->tail, memory_order_acquire);
    if (tail >= head) {
        return tail - head;
    } else {
        return pHeader->capacity - (head - tail);
    }
}

size_t acaRingQueueSpscCapacity(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    return getRingQueueLfHeader(queue)->capacity;
}

int acaRingQueueSpscEnqueue(void *queue, const void *elem) {
    if (queue == NULL || elem == NULL) {
        return 0;
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = getRingQueueLfHeader(queue);
    size_t tail     = ACA_RING_ATOMIC_LOAD(&pHeader->tail, memory_order_relaxed);
    size_t head     = ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_acquire);
    size_t nextTail = (tail + 1) & (pHeader->capacity - 1);

    if (nextTail == head) {
        switch (pHeader->type) {
            case ACA_RING_QUEUE_SPSC_REJECT_DS:
            case ACA_RING_QUEUE_SPSC_REJECT_POW2_DS:
                // reject new element, do nothing
                return 0;
            case ACA_RING_QUEUE_SPSC_ASSERT_DS:
            case ACA_RING_QUEUE_SPSC_ASSERT_POW2_DS:
                // assert failure
                assert(0 && "ring queue is full!");
                return 0; // non-debug builds this falls back to "reject" behavior
            default:
                assert(0 && "unreachable");
                return 0;
        }
    }

    char *pDataPtr = (char *)(pHeader + 1);
    memcpy(pDataPtr + (tail * pHeader->elemSize), elem, pHeader->elemSize);
    ACA_RING_ATOMIC_STORE(&pHeader->tail, nextTail, memory_order_release);
    return 1;
}

size_t acaRingQueueSpscDequeue(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = getRingQueueLfHeader(queue);
    size_t head = ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_relaxed);
    size_t tail = ACA_RING_ATOMIC_LOAD(&pHeader->tail, memory_order_acquire);
    if (head == tail) {
        return 0; // queue is empty
    }
    ACA_RING_ATOMIC_STORE(
        &pHeader->head, (head + 1) & (pHeader->capacity - 1), memory_order_release);
    return head;
}

size_t acaRingQueueSpscFront(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = getRingQueueLfHeader(queue);
    return ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_acquire);
}

int acaRingQueueSpscEmpty(void *queue) {
    if (queue == NULL) {
        return 1; // consider NULL queue as empty
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = getRingQueueLfHeader(queue);
    size_t head = ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_relaxed);
    size_t tail = ACA_RING_ATOMIC_LOAD(&pHeader->tail, memory_order_acquire);
    return head == tail;
}

int acaRingQueueSpscFull(void *queue) {
    if (queue == NULL) {
        return 0; // consider NULL queue as not full
    }
    aca_ring_queue_spsc_ds_header_t *pHeader = getRingQueueLfHeader(queue);
    size_t tail = ACA_RING_ATOMIC_LOAD(&pHeader->tail, memory_order_relaxed);
    size_t head = ACA_RING_ATOMIC_LOAD(&pHeader->head, memory_order_acquire);
    return ((tail + 1) & (pHeader->capacity - 1)) == head;
}

#endif // ACA_RING_DS_IMPLEMENTATION

#endif // ACA_RING_DS_H
