#include <thread>

#include "aca_ring_ds.h"
#include "gtest/gtest.h"

TEST(ring_buffer, fixed_capacity) {
    char buffer[ACA_RING_BUFFER_RESERVE(int, 8)];
    int *ringBuffer = (int *)buffer;
    acaRingBufferCreate(ringBuffer, 8);
    EXPECT_NE(ringBuffer, nullptr);

    for (int i = 0; i < acaRingBufferCapacity(ringBuffer); ++i) {
        ringBuffer[i] = i + 1;
    }

    for (int i = 0; i < acaRingBufferCapacity(ringBuffer); ++i) {
        EXPECT_EQ(ringBuffer[i], i + 1);
    }
}

TEST(ring_buffer, create_and_free) {
    int *buffer = nullptr;
    acaRingBufferCreate(buffer, 8);
    EXPECT_NE(buffer, nullptr);
    acaRingBufferFree(buffer);
}

TEST(ring_buffer, capacity_and_front) {
    int *buffer = nullptr;
    acaRingBufferCreate(buffer, 8);
    EXPECT_EQ(acaRingBufferCapacity(buffer), 8);
    EXPECT_EQ(acaRingBufferFront(buffer), 0);
    acaRingBufferFree(buffer);
}

TEST(ring_buffer, next) {
    int *buffer = nullptr;
    acaRingBufferCreate(buffer, 4);
    EXPECT_EQ(acaRingBufferFront(buffer), 0);
    acaRingBufferNext(buffer);
    EXPECT_EQ(acaRingBufferFront(buffer), 1);
    acaRingBufferNext(buffer);
    EXPECT_EQ(acaRingBufferFront(buffer), 2);
    acaRingBufferNext(buffer);
    EXPECT_EQ(acaRingBufferFront(buffer), 3);
    acaRingBufferNext(buffer);
    EXPECT_EQ(acaRingBufferFront(buffer), 0); // should wrap around
    acaRingBufferFree(buffer);
}

TEST(ring_queue, fixed_capacity) {
    char                    buffer[ACA_RING_QUEUE_RESERVE(float, 8)];
    float                  *ringQueue = (float *)buffer;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = false;
    acaRingQueueCreate(ringQueue, &config);
    EXPECT_NE(ringQueue, nullptr);

    float values[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    for (int i = 0; i < 6; ++i) {
        acaRingQueueEnqueue(ringQueue, &values[i]);
    }

    // we overwrote the first 4 elements, start there and dequeue
    for (int i = 3; i < 7; ++i) {
        if (!acaRingQueueEmpty(ringQueue)) {
            size_t frontIndex = acaRingQueueDequeue(ringQueue);
            EXPECT_EQ(frontIndex, (size_t)(i % 4));      // should return indices
            EXPECT_EQ(ringQueue[frontIndex], values[i]); // should return correct values
        }
    }
}

TEST(ring_queue, create_and_free) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);
    EXPECT_NE(queue, nullptr);
    acaRingQueueFree(queue);
}

TEST(ring_queue, size_and_capacity) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    EXPECT_EQ(acaRingQueueSize(queue), 0);
    EXPECT_EQ(acaRingQueueCapacity(queue), 8);

    acaRingQueueFree(queue);
}

TEST(ring_queue, empty_and_full) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    EXPECT_TRUE(acaRingQueueEmpty(queue));
    EXPECT_FALSE(acaRingQueueFull(queue));

    int values[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; ++i) {
        acaRingQueueEnqueue(queue, &values[i]);
    }

    EXPECT_FALSE(acaRingQueueEmpty(queue));
    EXPECT_TRUE(acaRingQueueFull(queue));

    acaRingQueueFree(queue);
}

TEST(ring_queue, enqueue_and_front) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    for (int i = 0; i < 3; ++i) {
        acaRingQueueEnqueue(queue, &i);
        size_t frontIndex = acaRingQueueFront(queue);
        EXPECT_EQ(frontIndex, 0);        // front should always be the first element
        EXPECT_EQ(queue[frontIndex], 0); // front value should always be the first element
    }

    acaRingQueueFree(queue);
}

TEST(ring_queue, enqueue_and_dequeue) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    for (int i = 0; i < 6; ++i) {
        acaRingQueueEnqueue(queue, &i);
    }

    // we overwrote the first 3 elements, start there and dequeue 4 elements
    for (int i = 3; i < 7; ++i) {
        if (!acaRingQueueEmpty(queue)) {
            size_t frontIndex = acaRingQueueDequeue(queue);
            EXPECT_EQ(frontIndex, (size_t)(i % 4)); // should return indices in order of insertion
            EXPECT_EQ(queue[frontIndex], i);        // should return correct values
        }
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(queue));

    acaRingQueueFree(queue);
}

TEST(ring_queue, full_behavior_reject) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    char                   *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    char values[] = {'a', 'b', 'c', 'd', 'e', 'f'};
    for (int i = 0; i < 6; ++i) {
        acaRingQueueEnqueue(queue, &values[i]);
    }

    // dequeue elements and check values
    for (int i = 0; i < 3; ++i) {
        size_t frontIndex = acaRingQueueDequeue(queue);
        EXPECT_EQ(frontIndex, (size_t)(i));      // should return indices in order of insertion
        EXPECT_EQ(queue[frontIndex], values[i]); // should return correct values
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(queue));

    acaRingQueueFree(queue);
}

TEST(ring_queue, full_behavior_overwrite) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    float                  *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    float values[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f};
    for (int i = 0; i < 6; ++i) {
        acaRingQueueEnqueue(queue, &values[i]);
    }

    // we overwrote the first 3 elements, start there and dequeue 4 elements
    for (int i = 3; i < 7; ++i) {
        if (!acaRingQueueEmpty(queue)) {
            size_t frontIndex = acaRingQueueDequeue(queue);
            EXPECT_EQ(frontIndex, (size_t)(i % 4));  // should return indices in order of insertion
            EXPECT_EQ(queue[frontIndex], values[i]); // should return correct values
        }
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(queue));

    acaRingQueueFree(queue);
}

TEST(ring_queue, full_behavior_assert) {
    // aca_ring_queue_ds impl is "waste-one-slot", we can only enqueue 3 items in a capacity of 4.
    unsigned int           *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_ASSERT;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    unsigned int values[] = {100, 200, 300, 400, 500};
    for (int i = 0; i < 3; ++i) {
        acaRingQueueEnqueue(queue, &values[i]);
    }

    // next enqueue should trigger assert failure
    ASSERT_DEATH(acaRingQueueEnqueue(queue, &values[4]), "ring queue is full!");

    // queue should still be full with the first 4 elements
    EXPECT_FALSE(acaRingQueueEmpty(queue));
    EXPECT_TRUE(acaRingQueueFull(queue));

    for (int i = 0; i < 3; ++i) {
        size_t frontIndex = acaRingQueueDequeue(queue);
        EXPECT_EQ(frontIndex, (size_t)(i));      // should return indices in order of insertion
        EXPECT_EQ(queue[frontIndex], values[i]); // should return correct values
    }

    // queue should be empty now
    EXPECT_TRUE(acaRingQueueEmpty(queue));

    acaRingQueueFree(queue);
}

TEST(ring_queue, dynamic_resize) {
    double                 *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    double values[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    for (int i = 0; i < 6; ++i) {
        if (acaRingQueueFull(queue)) {
            queue = (double *)acaRingQueueResize(queue, acaRingQueueCapacity(queue) * 2);
        }
        acaRingQueueEnqueue(queue, &values[i]);
    }
    for (int i = 0; i < 6; ++i) {
        size_t frontIndex = acaRingQueueDequeue(queue);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(queue[frontIndex], values[i]);
    }

    acaRingQueueFree(queue);
}

TEST(ring_queue, dynamic_enqueue_dequeue) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_OVERWRITE;
    config.isHeapAlloced = true;
    acaRingQueueCreate(queue, &config);

    // enqueue-and-dequeue around half of the capacity, then enqueue more to
    // trigger resize to test wrap-around logic in resizing
    for (int i = 0; i < 4; ++i) {
        EXPECT_NE(acaRingQueueFull(queue), true);
        acaRingQueueEnqueue(queue, &i);
        size_t frontIndex = acaRingQueueDequeue(queue);
        EXPECT_EQ(acaRingQueueEmpty(queue), true);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(queue[frontIndex], i);
    }

    int values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    for (int i = 0; i < 10; ++i) {
        if (acaRingQueueFull(queue)) {
            queue = (int *)acaRingQueueResize(queue, acaRingQueueCapacity(queue) * 2);
        }
        acaRingQueueEnqueue(queue, &values[i]);
    }

    for (int i = 0; i < 10; ++i) {
        size_t frontIndex = acaRingQueueDequeue(queue);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(queue[frontIndex], values[i]);
    }

    acaRingQueueFree(queue);
}

// ring_buffer_lf

TEST(ring_buffer_lf, fixed_capacity) {
    char buffer[ACA_RING_BUFFER_SPSC_RESERVE(int, 8)];
    int *ringBuffer = (int *)buffer;
    acaRingBufferSpscCreate(ringBuffer, 8);
    EXPECT_NE(ringBuffer, nullptr);

    for (int i = 0; i < acaRingBufferSpscCapacity(ringBuffer); ++i) {
        ringBuffer[i] = i + 1;
    }

    for (int i = 0; i < acaRingBufferSpscCapacity(ringBuffer); ++i) {
        EXPECT_EQ(ringBuffer[i], i + 1);
    }
}

TEST(ring_buffer_lf, create_rejects_zero_capacity) {
    char buffer[16];
    int *ringBuffer = (int *)buffer;
    EXPECT_EQ(acaRingBufferSpscCreate(ringBuffer, 0), nullptr);
}

TEST(ring_buffer_lf, create_rejects_zero_elem_size) {
    char *buffer = nullptr;
    EXPECT_EQ(acaRingBufferCreateSpscImpl(buffer, 0, 8), nullptr);
}

TEST(ring_buffer_lf, create_and_free) {
    int *buffer = nullptr;
    acaRingBufferSpscCreate(buffer, 8);
    EXPECT_NE(buffer, nullptr);
    acaRingBufferSpscFree(buffer);
}

TEST(ring_buffer_lf, null_handling) {
    EXPECT_EQ(acaRingBufferSpscCapacity(nullptr), 0);
    EXPECT_EQ(acaRingBufferSpscFront(nullptr), 0);
    acaRingBufferSpscNext(nullptr);
    acaRingBufferSpscFree(nullptr);
}

TEST(ring_buffer_lf, capacity_and_front) {
    int *buffer = nullptr;
    acaRingBufferSpscCreate(buffer, 8);
    EXPECT_EQ(acaRingBufferSpscCapacity(buffer), 8);
    EXPECT_EQ(acaRingBufferSpscFront(buffer), 0);
    acaRingBufferSpscFree(buffer);
}

TEST(ring_buffer_lf, next) {
    int *buffer = nullptr;
    acaRingBufferSpscCreate(buffer, 4);
    EXPECT_EQ(acaRingBufferSpscFront(buffer), 0);
    acaRingBufferSpscNext(buffer);
    EXPECT_EQ(acaRingBufferSpscFront(buffer), 1);
    acaRingBufferSpscNext(buffer);
    EXPECT_EQ(acaRingBufferSpscFront(buffer), 2);
    acaRingBufferSpscNext(buffer);
    EXPECT_EQ(acaRingBufferSpscFront(buffer), 3);
    acaRingBufferSpscNext(buffer);
    EXPECT_EQ(acaRingBufferSpscFront(buffer), 0); // should wrap around
    acaRingBufferSpscFree(buffer);
}

TEST(ring_buffer_lf, front_wraps_multiple_laps) {
    int *buffer = nullptr;
    acaRingBufferSpscCreate(buffer, 4);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(acaRingBufferSpscFront(buffer), (size_t)(i % 4));
        acaRingBufferSpscNext(buffer);
    }
    EXPECT_EQ(acaRingBufferSpscFront(buffer), (size_t)(10 % 4));
    acaRingBufferSpscFree(buffer);
}

// ring_queue_lf

TEST(ring_queue_lf, create_rejects_zero_capacity) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 0;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    EXPECT_EQ(acaRingQueueSpscCreate(queue, &config), nullptr);
}

TEST(ring_queue_lf, create_rejects_zero_elem_size) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    EXPECT_EQ(acaRingQueueCreateSpscImpl(queue, 0, &config), nullptr);
}

TEST(ring_queue_lf, create_and_free) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);
    EXPECT_NE(queue, nullptr);
    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, fixed_capacity) {
    char                    buffer[ACA_RING_QUEUE_SPSC_RESERVE(int, 8)];
    int                    *queue = (int *)buffer;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = false;
    acaRingQueueSpscCreate(queue, &config);
    EXPECT_NE(queue, nullptr);
    EXPECT_EQ(acaRingQueueSpscCapacity(queue), 8);
    EXPECT_TRUE(acaRingQueueSpscEmpty(queue));
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
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);

    EXPECT_EQ(acaRingQueueSpscSize(queue), 0);
    EXPECT_EQ(acaRingQueueSpscCapacity(queue), 8);

    int values[] = {1, 2, 3};
    for (int i = 0; i < 3; ++i) {
        EXPECT_NE(acaRingQueueSpscEnqueue(queue, &values[i]), 0);
    }
    EXPECT_EQ(acaRingQueueSpscSize(queue), 3);

    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, empty_and_full) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);

    EXPECT_TRUE(acaRingQueueSpscEmpty(queue));
    EXPECT_FALSE(acaRingQueueSpscFull(queue));

    int values[] = {1, 2, 3};
    for (int i = 0; i < 3; ++i) {
        EXPECT_NE(acaRingQueueSpscEnqueue(queue, &values[i]), 0);
    }

    EXPECT_FALSE(acaRingQueueSpscEmpty(queue));
    EXPECT_TRUE(acaRingQueueSpscFull(queue));

    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, reject_when_full) {
    // wastes one slot, so only 3 elements fit in a capacity of 4
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);

    int values[] = {1, 2, 3, 4, 5, 6};
    for (int i = 0; i < 3; ++i) {
        EXPECT_NE(acaRingQueueSpscEnqueue(queue, &values[i]), 0);
    }

    // queue is full, all further enqueues should be rejected
    for (int i = 3; i < 6; ++i) {
        EXPECT_EQ(acaRingQueueSpscEnqueue(queue, &values[i]), 0);
    }
    EXPECT_EQ(acaRingQueueSpscSize(queue), 3);

    for (int i = 0; i < 3; ++i) {
        size_t frontIndex = acaRingQueueSpscDequeue(queue);
        EXPECT_EQ(frontIndex, (size_t)(i));
        EXPECT_EQ(queue[frontIndex], values[i]);
    }
    EXPECT_TRUE(acaRingQueueSpscEmpty(queue));

    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, enqueue_and_front) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; ++i) {
        acaRingQueueSpscEnqueue(queue, &values[i]);
        size_t frontIndex = acaRingQueueSpscFront(queue);
        EXPECT_EQ(frontIndex, 0);
        EXPECT_EQ(queue[frontIndex], values[0]);
    }

    acaRingQueueSpscDequeue(queue);
    EXPECT_EQ(acaRingQueueSpscFront(queue), 1);
    EXPECT_EQ(queue[acaRingQueueSpscFront(queue)], values[1]);

    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, enqueue_and_dequeue) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);

    // enqueue and dequeue more elements than the capacity to force wrap-around
    for (int i = 0; i < 10; ++i) {
        EXPECT_NE(acaRingQueueSpscEnqueue(queue, &i), 0);
        size_t frontIndex = acaRingQueueSpscDequeue(queue);
        EXPECT_EQ(frontIndex, (size_t)(i % 4));
        EXPECT_EQ(queue[frontIndex], i);
    }

    EXPECT_TRUE(acaRingQueueSpscEmpty(queue));
    EXPECT_EQ(acaRingQueueSpscSize(queue), 0);

    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, dequeue_empty_returns_zero) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);
    EXPECT_EQ(acaRingQueueSpscDequeue(queue), 0);
    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, enqueue_null_elem_rejected) {
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 4;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);
    EXPECT_EQ(acaRingQueueSpscEnqueue(queue, nullptr), 0);
    EXPECT_TRUE(acaRingQueueSpscEmpty(queue));
    acaRingQueueSpscFree(queue);
}

TEST(ring_queue_lf, concurrent_single_producer_consumer) {
    // exercise the lock-free path with one producer and one consumer thread
    int                    *queue = nullptr;
    aca_ring_queue_config_t config;
    config.capacity      = 8;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = true;
    acaRingQueueSpscCreate(queue, &config);

    const int numElements = 1000;
    bool      producerOk  = false;
    bool      consumerOk  = false;

    std::thread producer([&]() {
        producerOk = true;
        for (int i = 0; i < numElements; ++i) {
            while (acaRingQueueSpscEnqueue(queue, &i) == 0) {
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
            if (acaRingQueueSpscEmpty(queue)) {
                continue;
            }
            size_t frontIndex = acaRingQueueSpscDequeue(queue);
            EXPECT_EQ(queue[frontIndex], received);
            ++received;
        }
    });

    producer.join();
    consumer.join();
    EXPECT_TRUE(producerOk);
    EXPECT_TRUE(consumerOk);
    EXPECT_TRUE(acaRingQueueSpscEmpty(queue));

    acaRingQueueSpscFree(queue);
}
