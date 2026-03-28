#ifndef ACA_RING_DS_H
#define ACA_RING_DS_H

#include <stddef.h>

typedef enum aca_ring_buffer_ds_type {
    ACA_RING_BUFFER_DS = 0,
    ACA_RING_BUFFER_POW2_DS,

} aca_ring_buffer_ds_type_t;

typedef struct aca_ring_buffer_ds_header {
    size_t                    size;
    size_t                    head;
    aca_ring_buffer_ds_type_t type;
} aca_ring_buffer_ds_header_t;

#define ACA_RING_BUFFER_RESERVE(elemSize, count)                                                   \
    ((count) * (elemSize) + sizeof(aca_ring_buffer_ds_header_t))
#define ACA_RING_RESERVE_FOR(T, count) ACA_RING_BUFFER_RESERVE(sizeof(T), (count))

// acaRingBuffer API
void  *acaRingBufferCreateImpl(void *buffer, size_t elemSize, size_t capacity);
void   acaRingBufferFree(void *buffer);
size_t acaRingBufferCapacity(void *buffer);
size_t acaRingBufferFront(void *buffer);
void   acaRingBufferNext(void *buffer);
#ifdef __cplusplus
template <typename T>
static T *acaRingBufferCreateCpp(T *buffer, size_t elemSize, size_t capacity) {
    return (T *)acaRingBufferCreateImpl(buffer, elemSize, capacity);
}
#define acaRingBufferCreate(T, size) ((T) = acaRingBufferCreateCpp((T), (sizeof(*(T))), (size)))
#else
#define acaRingBufferCreate(T, size) (T) = (acaRingBufferCreateImpl((T), (sizeof(*(T))), (size)))
#endif // __cplusplus

typedef enum aca_ring_queue_ds_type {
    ACA_RING_QUEUE_FIXED_OVERWRITE_DS,
    ACA_RING_QUEUE_FIXED_OVERWRITE_POW2_DS,
    ACA_RING_QUEUE_FIXED_REJECT_DS,
    ACA_RING_QUEUE_FIXED_REJECT_POW2_DS,
    ACA_RING_QUEUE_FIXED_ASSERT_DS,
    ACA_RING_QUEUE_FIXED_ASSERT_POW2_DS,
    ACA_RING_QUEUE_DYNAMIC_DS,
    ACA_RING_QUEUE_DYNAMIC_POW2_DS,
} aca_ring_queue_ds_type_t;

typedef enum aca_ring_queue_ds_full_behavior {
    ACA_RING_QUEUE_OVERWRITE,
    ACA_RING_QUEUE_REJECT,
    ACA_RING_QUEUE_ASSERT,
    ACA_RING_QUEUE_RESIZE,
} aca_ring_queue_ds_full_behavior_t;

typedef struct aca_ring_queue_ds_header {
    size_t                   size;
    size_t                   elemSize;
    size_t                   head;
    size_t                   tail;
    aca_ring_queue_ds_type_t type;
} aca_ring_queue_ds_header_t;

#define ACA_RING_QUEUE_RESERVE(elemSize, count)                                                    \
    ((count) * (sizeof(elemSize)) + sizeof(aca_ring_queue_ds_header_t))
#define ACA_RING_QUEUE_RESERVE_FOR(T, count) ACA_RING_QUEUE_RESERVE(sizeof(T), (count))

typedef struct aca_ring_queue_ds_config {
    size_t                            capacity;
    aca_ring_queue_ds_full_behavior_t fullBehavior;
} aca_ring_queue_config_t;

// acaRingQueue API
void  *acaRingQueueCreateImpl(void *queue, size_t elemSize, const aca_ring_queue_config_t *config);
void   acaRingQueueFree(void *queue);
size_t acaRingQueueSize(void *queue);
size_t acaRingQueueCapacity(void *queue);
void   acaRingQueueEnqueue(void *queue, const void *elem);
size_t acaRingQueueDequeue(void *queue);
size_t acaRingQueueFront(void *queue);
int    acaRingQueueEmpty(void *queue);
int    acaRingQueueFull(void *queue);
#ifdef __cplusplus
template <typename T>
static T *acaRingQueueCreateCpp(T *queue, size_t elemSize, const aca_ring_queue_config_t *config) {
    return (T *)acaRingQueueCreateImpl(queue, elemSize, config);
}
#define acaRingQueueCreate(T, config) ((T) = acaRingQueueCreateCpp((T), (sizeof(*(T))), (config)))
#else
#define acaRingQueueCreate(T, config) (T) = (acaRingQueueCreateImpl((T), (sizeof(*(T))), (config)))
#endif // __cplusplus

#ifdef ACA_RING_DS_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline aca_ring_buffer_ds_header_t *GetRingBufferHeader(void *buffer) {
    return ((aca_ring_buffer_ds_header_t *)buffer) - 1;
}

static inline int IsPow2(size_t x) {
    return (x > 0) && ((x & (x - 1)) == 0);
}

static inline aca_ring_queue_ds_header_t *GetRingQueueHeader(void *queue) {
    return ((aca_ring_queue_ds_header_t *)queue) - 1;
}

static inline size_t FindNextRingQueueIndex(aca_ring_queue_ds_header_t *header, size_t index) {
    switch (header->type) {
        case ACA_RING_QUEUE_DYNAMIC_POW2_DS:
        case ACA_RING_QUEUE_FIXED_ASSERT_POW2_DS:
        case ACA_RING_QUEUE_FIXED_REJECT_POW2_DS:
        case ACA_RING_QUEUE_FIXED_OVERWRITE_POW2_DS:
            index = (index + 1) & (header->size - 1);
            break;
        default:
            index = (index + 1) % header->size;
            break;
    }
    return index;
}

void *acaRingBufferCreateImpl(void *buffer, size_t elemSize, size_t capacity) {
    aca_ring_buffer_ds_header_t *header;
    if (buffer == NULL) {
        header = (aca_ring_buffer_ds_header_t *)malloc(ACA_RING_BUFFER_RESERVE(elemSize, capacity));
        if (header == NULL) {
            return NULL;
        }
    } else {
        header = (aca_ring_buffer_ds_header_t *)buffer;
    }
    header->size = capacity;
    header->head = 0;

    if ((capacity > 0) && ((capacity & (capacity - 1)) == 0)) {
        header->type = ACA_RING_BUFFER_POW2_DS;
    } else {
        header->type = ACA_RING_BUFFER_DS;
    }

    return (header + 1); // return pointer to data, not header
}

void acaRingBufferFree(void *buffer) {
    if (buffer == NULL) {
        return;
    }
    free(GetRingBufferHeader(buffer));
}

size_t acaRingBufferCapacity(void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    return GetRingBufferHeader(buffer)->size;
}

size_t acaRingBufferFront(void *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    aca_ring_buffer_ds_header_t *header = GetRingBufferHeader(buffer);
    return header->head;
}

void acaRingBufferNext(void *buffer) {
    if (buffer == NULL) {
        return;
    }
    aca_ring_buffer_ds_header_t *header = GetRingBufferHeader(buffer);
    switch (header->type) {
        case ACA_RING_BUFFER_POW2_DS:
            header->head = (header->head + 1) & (header->size - 1);
            break;
        case ACA_RING_BUFFER_DS:
            header->head = (header->head + 1) % (header->size);
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
    aca_ring_queue_ds_header_t *header;
    if (queue == NULL) {
        header = (aca_ring_queue_ds_header_t *)malloc(
            ACA_RING_QUEUE_RESERVE(elemSize, config->capacity));
        if (header == NULL) {
            return NULL;
        }
    } else {
        header = (aca_ring_queue_ds_header_t *)queue;
    }
    header->size     = config->capacity;
    header->elemSize = elemSize;
    header->head     = 0;
    header->tail     = 0;

    const int isCapacityPow2 = IsPow2(config->capacity);
    if (isCapacityPow2) {
        switch (config->fullBehavior) {
            case ACA_RING_QUEUE_OVERWRITE:
                header->type = ACA_RING_QUEUE_FIXED_OVERWRITE_POW2_DS;
                break;
            case ACA_RING_QUEUE_REJECT:
                header->type = ACA_RING_QUEUE_FIXED_REJECT_POW2_DS;
                break;
            case ACA_RING_QUEUE_ASSERT:
                header->type = ACA_RING_QUEUE_FIXED_ASSERT_POW2_DS;
                break;
            case ACA_RING_QUEUE_RESIZE:
                header->type = ACA_RING_QUEUE_DYNAMIC_POW2_DS;
                break;
            default:
                assert(0 && "unknown full behavior!");
                break;
        }
    } else {
        switch (config->fullBehavior) {
            case ACA_RING_QUEUE_OVERWRITE:
                header->type = ACA_RING_QUEUE_FIXED_OVERWRITE_DS;
                break;
            case ACA_RING_QUEUE_REJECT:
                header->type = ACA_RING_QUEUE_FIXED_REJECT_DS;
                break;
            case ACA_RING_QUEUE_ASSERT:
                header->type = ACA_RING_QUEUE_FIXED_ASSERT_DS;
                break;
            case ACA_RING_QUEUE_RESIZE:
                header->type = ACA_RING_QUEUE_DYNAMIC_DS;
                break;
            default:
                assert(0 && "unknown full behavior!");
                break;
        }
    }

    return (header + 1); // return pointer to data, not header
}

void acaRingQueueFree(void *queue) {
    if (queue == NULL) {
        return;
    }
    free(GetRingQueueHeader(queue));
}

size_t acaRingQueueSize(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    aca_ring_queue_ds_header_t *header = GetRingQueueHeader(queue);
    if (header->tail >= header->head) {
        return header->tail - header->head;
    } else {
        return header->size - (header->head - header->tail);
    }
}

size_t acaRingQueueCapacity(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    return GetRingQueueHeader(queue)->size;
}

void acaRingQueueEnqueue(void *queue, const void *elem) {
    if (queue == NULL || elem == NULL) {
        return;
    }
    aca_ring_queue_ds_header_t *header = GetRingQueueHeader(queue);
    if (acaRingQueueFull(queue)) {
        switch (header->type) {
            case ACA_RING_QUEUE_FIXED_OVERWRITE_DS:
            case ACA_RING_QUEUE_FIXED_OVERWRITE_POW2_DS:
                // overwrite the oldest element
                header->head = FindNextRingQueueIndex(header, header->head);
                break;
            case ACA_RING_QUEUE_FIXED_REJECT_DS:
            case ACA_RING_QUEUE_FIXED_REJECT_POW2_DS:
                // reject new element, do nothing
                return;
            case ACA_RING_QUEUE_FIXED_ASSERT_DS:
            case ACA_RING_QUEUE_FIXED_ASSERT_POW2_DS:
                // assert failure
                assert(0 && "ring queue is full!");
                return;
            default:
                // todo: for dynamic resizing, we can implement it here in the future
                assert(0 && "invalid ring queue type!");
                return;
        }
    }

    char  *dataPtr = (char *)(header + 1);
    size_t offset  = header->tail * header->elemSize;
    memcpy(dataPtr + offset, elem, header->elemSize);

    header->tail = FindNextRingQueueIndex(header, header->tail);
}

size_t acaRingQueueDequeue(void *queue) {
    if (queue == NULL) {
        return 0;
    }

    aca_ring_queue_ds_header_t *header = GetRingQueueHeader(queue);
    if (acaRingQueueEmpty(queue)) {
        return 0; // queue is empty
    }

    size_t frontIndex = header->head;
    header->head      = FindNextRingQueueIndex(header, header->head);

    return frontIndex;
}

size_t acaRingQueueFront(void *queue) {
    if (queue == NULL) {
        return 0;
    }
    return GetRingQueueHeader(queue)->head;
}

int acaRingQueueEmpty(void *queue) {
    if (queue == NULL) {
        return 1; // consider NULL queue as empty
    }
    aca_ring_queue_ds_header_t *header = GetRingQueueHeader(queue);
    return header->head == header->tail;
}

int acaRingQueueFull(void *queue) {
    if (queue == NULL) {
        return 0; // consider NULL queue as not full
    }
    aca_ring_queue_ds_header_t *header = GetRingQueueHeader(queue);
    return FindNextRingQueueIndex(header, header->tail) == header->head;
}

#endif // ACA_RING_DS_IMPLEMENTATION

#endif // ACA_RING_DS_H
