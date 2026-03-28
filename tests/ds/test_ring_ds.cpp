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
    aca_ring_queue_config_t config    = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_OVERWRITE};
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
    int                    *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 8, .fullBehavior = ACA_RING_QUEUE_REJECT};
    acaRingQueueCreate(queue, &config);
    EXPECT_NE(queue, nullptr);
    acaRingQueueFree(queue);
}

TEST(ring_queue, size_and_capacity) {
    int                    *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 8, .fullBehavior = ACA_RING_QUEUE_REJECT};
    acaRingQueueCreate(queue, &config);

    EXPECT_EQ(acaRingQueueSize(queue), 0);
    EXPECT_EQ(acaRingQueueCapacity(queue), 8);

    acaRingQueueFree(queue);
}

TEST(ring_queue, empty_and_full) {
    int                    *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_OVERWRITE};
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
    int                    *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_OVERWRITE};
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
    int                    *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_OVERWRITE};
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
    char                   *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_REJECT};
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
    float                  *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_OVERWRITE};
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
    unsigned int           *queue  = nullptr;
    aca_ring_queue_config_t config = {.capacity = 4, .fullBehavior = ACA_RING_QUEUE_ASSERT};
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
