#include "aca_ring_ds.h"
#include "gtest/gtest.h"

#include <cstddef>
#include <thread>

#include <stdint.h>

TEST(ring_buffer, fixed_capacity) {
    char buffer[ACA_RING_BUFFER_RESERVE(int, 8)];
    int *pRingBuffer = (int *)buffer;
    acaRingBufferCreate(pRingBuffer, 8);
    EXPECT_NE(pRingBuffer, nullptr);

    for (size_t i = 0; i < acaRingBufferCapacity(pRingBuffer); ++i) {
        pRingBuffer[i] = static_cast<int>(i) + 1;
    }

    for (size_t i = 0; i < acaRingBufferCapacity(pRingBuffer); ++i) {
        EXPECT_EQ(pRingBuffer[i], i + 1);
    }
}

TEST(ring_buffer, create_and_free) {
    int *pBuffer = nullptr;
    acaRingBufferCreate(pBuffer, 8);
    EXPECT_NE(pBuffer, nullptr);
    acaRingBufferFree(pBuffer);
}

TEST(ring_buffer, capacity_and_front) {
    int *pBuffer = nullptr;
    acaRingBufferCreate(pBuffer, 8);
    EXPECT_EQ(acaRingBufferCapacity(pBuffer), 8);
    EXPECT_EQ(acaRingBufferFront(pBuffer), 0);
    acaRingBufferFree(pBuffer);
}

TEST(ring_buffer, next) {
    int *pBuffer = nullptr;
    acaRingBufferCreate(pBuffer, 4);
    EXPECT_EQ(acaRingBufferFront(pBuffer), 0);
    acaRingBufferNext(pBuffer);
    EXPECT_EQ(acaRingBufferFront(pBuffer), 1);
    acaRingBufferNext(pBuffer);
    EXPECT_EQ(acaRingBufferFront(pBuffer), 2);
    acaRingBufferNext(pBuffer);
    EXPECT_EQ(acaRingBufferFront(pBuffer), 3);
    acaRingBufferNext(pBuffer);
    EXPECT_EQ(acaRingBufferFront(pBuffer), 0); // should wrap around
    acaRingBufferFree(pBuffer);
}

TEST(ring_queue, fixed_capacity) {
    char                    buffer[ACA_RING_QUEUE_RESERVE(float, 8)];
    float                  *pRingQueue = (float *)buffer;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = false;
    acaRingQueueCreate(pRingQueue, &config);
    EXPECT_NE(pRingQueue, nullptr);

    float values[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    for (float &value : values) {
        acaRingQueueEnqueue(pRingQueue, &value);
    }

    // we overwrote the first 4 elements, start there and dequeue
    for (int i = 3; i < 7; ++i) {
        if (!acaRingQueueEmpty(pRingQueue)) {
            size_t frontIndex = acaRingQueueDequeue(pRingQueue);
            EXPECT_EQ(frontIndex, (size_t)(i % 4));       // should return indices
            EXPECT_EQ(pRingQueue[frontIndex], values[i]); // should return correct values
        }
    }
}

TEST(ring_queue, create_and_free) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);
    EXPECT_NE(pQueue, nullptr);
    acaRingQueueFree(pQueue);
}

TEST(ring_queue, size_and_capacity) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    EXPECT_EQ(acaRingQueueSize(pQueue), 0);
    EXPECT_EQ(acaRingQueueCapacity(pQueue), 8);

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, empty_and_full) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    EXPECT_TRUE(acaRingQueueEmpty(pQueue));
    EXPECT_FALSE(acaRingQueueFull(pQueue));

    int values[] = {1, 2, 3, 4};
    for (int &value : values) {
        acaRingQueueEnqueue(pQueue, &value);
    }

    EXPECT_FALSE(acaRingQueueEmpty(pQueue));
    EXPECT_TRUE(acaRingQueueFull(pQueue));

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, enqueue_and_front) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    for (int i = 0; i < 3; ++i) {
        acaRingQueueEnqueue(pQueue, &i);
        size_t frontIndex = acaRingQueueFront(pQueue);
        EXPECT_EQ(frontIndex, 0);         // front should always be the first element
        EXPECT_EQ(pQueue[frontIndex], 0); // front value should always be the first element
    }

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, enqueue_and_dequeue) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    for (int i = 0; i < 6; ++i) {
        acaRingQueueEnqueue(pQueue, &i);
    }

    // we overwrote the first 3 elements, start there and dequeue 4 elements
    for (int i = 3; i < 7; ++i) {
        if (!acaRingQueueEmpty(pQueue)) {
            size_t frontIndex = acaRingQueueDequeue(pQueue);
            EXPECT_EQ(frontIndex, (size_t)(i % 4)); // should return indices in order of insertion
            EXPECT_EQ(pQueue[frontIndex], i);       // should return correct values
        }
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(pQueue));

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, full_behavior_reject) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    char                   *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    char values[] = {'a', 'b', 'c', 'd', 'e', 'f'};
    for (char &value : values) {
        acaRingQueueEnqueue(pQueue, &value);
    }

    // dequeue elements and check values
    for (int i = 0; i < 3; ++i) {
        size_t frontIndex = acaRingQueueDequeue(pQueue);
        EXPECT_EQ(frontIndex, (size_t)(i));       // should return indices in order of insertion
        EXPECT_EQ(pQueue[frontIndex], values[i]); // should return correct values
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(pQueue));

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, full_behavior_overwrite) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    float                  *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    float values[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f};
    for (float &value : values) {
        acaRingQueueEnqueue(pQueue, &value);
    }

    // we overwrote the first 3 elements, start there and dequeue 4 elements
    for (int i = 3; i < 7; ++i) {
        if (!acaRingQueueEmpty(pQueue)) {
            size_t frontIndex = acaRingQueueDequeue(pQueue);
            EXPECT_EQ(frontIndex, (size_t)(i % 4));   // should return indices in order of insertion
            EXPECT_EQ(pQueue[frontIndex], values[i]); // should return correct values
        }
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(pQueue));

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, full_behavior_assert) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    unsigned int           *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_ASSERT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    unsigned int values[] = {100, 200, 300, 400, 500};
    for (int i = 0; i < 3; ++i) {
        acaRingQueueEnqueue(pQueue, &values[i]);
    }

    // next enqueue should trigger assert failure
    ASSERT_DEATH(acaRingQueueEnqueue(pQueue, &values[4]), "ring queue is full!");

    // queue should still be full with the first 4 elements
    EXPECT_FALSE(acaRingQueueEmpty(pQueue));
    EXPECT_TRUE(acaRingQueueFull(pQueue));

    for (int i = 0; i < 3; ++i) {
        size_t frontIndex = acaRingQueueDequeue(pQueue);
        EXPECT_EQ(frontIndex, (size_t)(i));       // should return indices in order of insertion
        EXPECT_EQ(pQueue[frontIndex], values[i]); // should return correct values
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(pQueue));

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, dynamic_resize) {
    double                 *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    double values[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    for (double &value : values) {
        if (acaRingQueueFull(pQueue)) {
            pQueue = (double *)acaRingQueueResize(pQueue, acaRingQueueCapacity(pQueue) * 2);
        }
        acaRingQueueEnqueue(pQueue, &value);
    }
    for (int i = 0; i < 6; ++i) {
        size_t frontIndex = acaRingQueueDequeue(pQueue);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(pQueue[frontIndex], values[i]);
    }

    acaRingQueueFree(pQueue);
}

TEST(ring_queue, dynamic_enqueue_dequeue) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(pQueue, &config);

    // enqueue-and-dequeue around half of the capacity, then enqueue more to
    // trigger resize to test wrap-around logic in resizing
    for (int i = 0; i < 4; ++i) {
        EXPECT_NE(acaRingQueueFull(pQueue), true);
        acaRingQueueEnqueue(pQueue, &i);
        size_t frontIndex = acaRingQueueDequeue(pQueue);
        EXPECT_EQ(acaRingQueueEmpty(pQueue), true);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(pQueue[frontIndex], i);
    }

    int values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    for (int &value : values) {
        if (acaRingQueueFull(pQueue)) {
            pQueue = (int *)acaRingQueueResize(pQueue, acaRingQueueCapacity(pQueue) * 2);
        }
        acaRingQueueEnqueue(pQueue, &value);
    }

    for (int i = 0; i < 10; ++i) {
        size_t frontIndex = acaRingQueueDequeue(pQueue);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(pQueue[frontIndex], values[i]);
    }

    acaRingQueueFree(pQueue);
}

// ring_buffer_lf

TEST(ring_buffer_lf, fixed_capacity) {
    char buffer[ACA_RING_BUFFER_SPSC_RESERVE(int, 8)];
    int *pRingBuffer = (int *)buffer;
    acaRingBufferSpscCreate(pRingBuffer, 8);
    EXPECT_NE(pRingBuffer, nullptr);

    for (size_t i = 0; i < acaRingBufferSpscCapacity(pRingBuffer); ++i) {
        pRingBuffer[i] = static_cast<int>(i) + 1;
    }

    for (size_t i = 0; i < acaRingBufferSpscCapacity(pRingBuffer); ++i) {
        EXPECT_EQ(pRingBuffer[i], i + 1);
    }
}

TEST(ring_buffer_lf, create_rejects_zero_capacity) {
    char buffer[16];
    int *pRingBuffer = (int *)buffer;
    EXPECT_EQ(acaRingBufferSpscCreate(pRingBuffer, 0), nullptr);
}

TEST(ring_buffer_lf, create_rejects_zero_elem_size) {
    char *pBuffer = nullptr;
    EXPECT_EQ(acaRingBufferCreateSpscImpl(pBuffer, 0, 8), nullptr);
}

TEST(ring_buffer_lf, create_and_free) {
    int *pBuffer = nullptr;
    acaRingBufferSpscCreate(pBuffer, 8);
    EXPECT_NE(pBuffer, nullptr);
    acaRingBufferSpscFree(pBuffer);
}

TEST(ring_buffer_lf, null_handling) {
    EXPECT_EQ(acaRingBufferSpscCapacity(nullptr), 0);
    EXPECT_EQ(acaRingBufferSpscFront(nullptr), 0);
    acaRingBufferSpscNext(nullptr);
    acaRingBufferSpscFree(nullptr);
}

TEST(ring_buffer_lf, capacity_and_front) {
    int *pBuffer = nullptr;
    acaRingBufferSpscCreate(pBuffer, 8);
    EXPECT_EQ(acaRingBufferSpscCapacity(pBuffer), 8);
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), 0);
    acaRingBufferSpscFree(pBuffer);
}

TEST(ring_buffer_lf, next) {
    int *pBuffer = nullptr;
    acaRingBufferSpscCreate(pBuffer, 4);
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), 0);
    acaRingBufferSpscNext(pBuffer);
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), 1);
    acaRingBufferSpscNext(pBuffer);
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), 2);
    acaRingBufferSpscNext(pBuffer);
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), 3);
    acaRingBufferSpscNext(pBuffer);
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), 0); // should wrap around
    acaRingBufferSpscFree(pBuffer);
}

TEST(ring_buffer_lf, front_wraps_multiple_laps) {
    int *pBuffer = nullptr;
    acaRingBufferSpscCreate(pBuffer, 4);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(acaRingBufferSpscFront(pBuffer), (size_t)(i % 4));
        acaRingBufferSpscNext(pBuffer);
    }
    EXPECT_EQ(acaRingBufferSpscFront(pBuffer), (size_t)(10 % 4));
    acaRingBufferSpscFree(pBuffer);
}

// ring_queue_lf

TEST(ring_queue_lf, create_rejects_zero_capacity) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 0;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    EXPECT_EQ(acaRingQueueSpscCreate(pQueue, &config), nullptr);
}

TEST(ring_queue_lf, create_rejects_zero_elem_size) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    EXPECT_EQ(acaRingQueueCreateSpscImpl(pQueue, 0, &config), nullptr);
}

TEST(ring_queue_lf, create_and_free) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);
    EXPECT_NE(pQueue, nullptr);
    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, fixed_capacity) {
    char                    buffer[ACA_RING_QUEUE_SPSC_RESERVE(int, 8)];
    int                    *pQueue = (int *)buffer;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = false;
    acaRingQueueSpscCreate(pQueue, &config);
    EXPECT_NE(pQueue, nullptr);
    EXPECT_EQ(acaRingQueueSpscCapacity(pQueue), 8);
    EXPECT_TRUE(acaRingQueueSpscEmpty(pQueue));
}

TEST(ring_queue_lf, null_handling) {
    EXPECT_EQ(acaRingQueueSpscSize(nullptr), 0);
    EXPECT_EQ(acaRingQueueSpscCapacity(nullptr), 0);
    EXPECT_EQ(acaRingQueueSpscEnqueue(nullptr, nullptr), 0);
    EXPECT_EQ(acaRingQueueSpscDequeue(nullptr), 0);
    EXPECT_EQ(acaRingQueueSpscFront(nullptr), 0);
    EXPECT_TRUE(acaRingQueueSpscEmpty(nullptr));
    EXPECT_FALSE(acaRingQueueSpscFull(nullptr));
    acaRingQueueSpscFree(nullptr);
}

TEST(ring_queue_lf, size_and_capacity) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);

    EXPECT_EQ(acaRingQueueSpscSize(pQueue), 0);
    EXPECT_EQ(acaRingQueueSpscCapacity(pQueue), 8);

    int values[] = {1, 2, 3};
    for (int &value : values) {
        EXPECT_NE(acaRingQueueSpscEnqueue(pQueue, &value), 0);
    }
    EXPECT_EQ(acaRingQueueSpscSize(pQueue), 3);

    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, empty_and_full) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);

    EXPECT_TRUE(acaRingQueueSpscEmpty(pQueue));
    EXPECT_FALSE(acaRingQueueSpscFull(pQueue));

    int values[] = {1, 2, 3};
    for (int &value : values) {
        EXPECT_NE(acaRingQueueSpscEnqueue(pQueue, &value), 0);
    }

    EXPECT_FALSE(acaRingQueueSpscEmpty(pQueue));
    EXPECT_TRUE(acaRingQueueSpscFull(pQueue));

    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, reject_when_full) {
    // wastes one slot, so only 3 elements fit in a capacity of 4
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);

    int values[] = {1, 2, 3, 4, 5, 6};
    for (int i = 0; i < 3; ++i) {
        EXPECT_NE(acaRingQueueSpscEnqueue(pQueue, &values[i]), 0);
    }

    // queue is full, all further enqueues should be rejected
    for (int i = 3; i < 6; ++i) {
        EXPECT_EQ(acaRingQueueSpscEnqueue(pQueue, &values[i]), 0);
    }
    EXPECT_EQ(acaRingQueueSpscSize(pQueue), 3);

    for (int i = 0; i < 3; ++i) {
        size_t frontIndex = acaRingQueueSpscDequeue(pQueue);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(pQueue[frontIndex], values[i]);
    }
    EXPECT_TRUE(acaRingQueueSpscEmpty(pQueue));

    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, enqueue_and_front) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);

    int values[] = {10, 20, 30};
    for (int &value : values) {
        acaRingQueueSpscEnqueue(pQueue, &value);
        size_t frontIndex = acaRingQueueSpscFront(pQueue);
        EXPECT_EQ(frontIndex, 0);
        EXPECT_EQ(pQueue[frontIndex], values[0]);
    }

    acaRingQueueSpscDequeue(pQueue);
    EXPECT_EQ(acaRingQueueSpscFront(pQueue), 1);
    EXPECT_EQ(pQueue[acaRingQueueSpscFront(pQueue)], values[1]);

    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, enqueue_and_dequeue) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);

    // enqueue and dequeue more elements than the capacity to force wrap-around
    for (int i = 0; i < 10; ++i) {
        EXPECT_NE(acaRingQueueSpscEnqueue(pQueue, &i), 0);
        size_t frontIndex = acaRingQueueSpscDequeue(pQueue);
        EXPECT_EQ(frontIndex, (size_t)(i % 4));
        EXPECT_EQ(pQueue[frontIndex], i);
    }

    EXPECT_TRUE(acaRingQueueSpscEmpty(pQueue));
    EXPECT_EQ(acaRingQueueSpscSize(pQueue), 0);

    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, dequeue_empty_returns_zero) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);
    EXPECT_EQ(acaRingQueueSpscDequeue(pQueue), 0);
    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, enqueue_null_elem_rejected) {
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);
    EXPECT_EQ(acaRingQueueSpscEnqueue(pQueue, nullptr), 0);
    EXPECT_TRUE(acaRingQueueSpscEmpty(pQueue));
    acaRingQueueSpscFree(pQueue);
}

TEST(ring_queue_lf, concurrent_single_producer_consumer) {
    // exercise the lock-free path with one producer and one consumer thread
    int                    *pQueue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(pQueue, &config);

    const int numElements = 1000;
    bool      producerOk  = false;
    bool      consumerOk  = false;

    std::thread producer([&]() {
        producerOk = true;
        for (int i = 0; i < numElements; ++i) {
            while (acaRingQueueSpscEnqueue(pQueue, &i) == 0) {
                // queue full, wait for the consumer to drain it
            }
        }
    });

    std::thread consumer([&]() {
        consumerOk   = true;
        int received = 0;
        while (received < numElements) {
            // only this thread dequeues, so a non-empty check here means the
            // dequeue below is guaranteed to succeed
            if (acaRingQueueSpscEmpty(pQueue)) {
                continue;
            }
            size_t frontIndex = acaRingQueueSpscDequeue(pQueue);
            EXPECT_EQ(pQueue[frontIndex], received);
            ++received;
        }
    });

    producer.join();
    consumer.join();
    EXPECT_TRUE(producerOk);
    EXPECT_TRUE(consumerOk);
    EXPECT_TRUE(acaRingQueueSpscEmpty(pQueue));

    acaRingQueueSpscFree(pQueue);
}
