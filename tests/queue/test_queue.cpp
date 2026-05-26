#include <gtest/gtest.h>

extern "C" {
#include "cutesim/queue.h"
}

#include "../test_describe.h"

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

TEST(Init, NewQueueIsEmpty) {
    DESCRIBE("a freshly initialised queue reports itself as empty");
    Queue q;
    queue_init(&q);
    EXPECT_TRUE(queue_is_empty(&q));
}

TEST(Init, NewQueueSizeIsZero) {
    DESCRIBE("a freshly initialised queue has size 0");
    Queue q;
    queue_init(&q);
    EXPECT_EQ(queue_size(&q), 0);
}

// ---------------------------------------------------------------------------
// Enqueue
// ---------------------------------------------------------------------------

TEST(Enqueue, EnqueueIncreasesSize) {
    DESCRIBE("enqueuing one element increments the size from 0 to 1");
    Queue q;
    queue_init(&q);
    int val = 42;
    queue_enqueue(&q, &val);
    EXPECT_EQ(queue_size(&q), 1);
    queue_destroy(&q);
}

TEST(Enqueue, EnqueuedItemIsAtFront) {
    DESCRIBE("after a single enqueue the peeked item equals the one inserted");
    Queue q;
    queue_init(&q);
    int val = 7;
    queue_enqueue(&q, &val);
    EXPECT_EQ(queue_peek(&q), &val);
    queue_destroy(&q);
}

TEST(Enqueue, QueueIsNotEmptyAfterEnqueue) {
    DESCRIBE("a queue with one element no longer reports itself as empty");
    Queue q;
    queue_init(&q);
    int val = 1;
    queue_enqueue(&q, &val);
    EXPECT_FALSE(queue_is_empty(&q));
    queue_destroy(&q);
}

// ---------------------------------------------------------------------------
// Dequeue
// ---------------------------------------------------------------------------

TEST(Dequeue, DequeueFromEmptyReturnsNull) {
    DESCRIBE("dequeuing from an empty queue returns NULL without crashing");
    Queue q;
    queue_init(&q);
    EXPECT_EQ(queue_dequeue(&q), nullptr);
}

TEST(Dequeue, DequeueReturnsFirstItem) {
    DESCRIBE("dequeue returns the pointer that was enqueued");
    Queue q;
    queue_init(&q);
    int val = 99;
    queue_enqueue(&q, &val);
    void *out = queue_dequeue(&q);
    EXPECT_EQ(out, &val);
}

TEST(Dequeue, DequeueDecreasesSize) {
    DESCRIBE("dequeuing one element from a single-item queue reduces size to 0");
    Queue q;
    queue_init(&q);
    int val = 5;
    queue_enqueue(&q, &val);
    queue_dequeue(&q);
    EXPECT_EQ(queue_size(&q), 0);
}

TEST(Dequeue, QueueIsEmptyAfterLastDequeue) {
    DESCRIBE("after dequeuing the only element the queue reports empty");
    Queue q;
    queue_init(&q);
    int val = 3;
    queue_enqueue(&q, &val);
    queue_dequeue(&q);
    EXPECT_TRUE(queue_is_empty(&q));
}

// ---------------------------------------------------------------------------
// Peek
// ---------------------------------------------------------------------------

TEST(Peek, PeekOnEmptyReturnsNull) {
    DESCRIBE("peeking into an empty queue returns NULL without crashing");
    Queue q;
    queue_init(&q);
    EXPECT_EQ(queue_peek(&q), nullptr);
}

TEST(Peek, PeekDoesNotRemoveItem) {
    DESCRIBE("peeking does not change the size of the queue");
    Queue q;
    queue_init(&q);
    int val = 11;
    queue_enqueue(&q, &val);
    queue_peek(&q);
    EXPECT_EQ(queue_size(&q), 1);
    queue_destroy(&q);
}

TEST(Peek, PeekReturnsFrontPointer) {
    DESCRIBE("peek returns the same pointer that was inserted first");
    Queue q;
    queue_init(&q);
    int a = 1, b = 2;
    queue_enqueue(&q, &a);
    queue_enqueue(&q, &b);
    EXPECT_EQ(queue_peek(&q), &a);
    queue_destroy(&q);
}

// ---------------------------------------------------------------------------
// FIFO order
// ---------------------------------------------------------------------------

TEST(FIFO, FIFOOrderPreserved) {
    DESCRIBE("elements come out in the same order they were inserted");
    Queue q;
    queue_init(&q);
    int a = 1, b = 2, c = 3;
    queue_enqueue(&q, &a);
    queue_enqueue(&q, &b);
    queue_enqueue(&q, &c);
    EXPECT_EQ(queue_dequeue(&q), &a);
    EXPECT_EQ(queue_dequeue(&q), &b);
    EXPECT_EQ(queue_dequeue(&q), &c);
}

// ---------------------------------------------------------------------------
// Size tracking
// ---------------------------------------------------------------------------

TEST(Size, SizeTracksCorrectly) {
    DESCRIBE("size increases with each enqueue and decreases with each dequeue");
    Queue q;
    queue_init(&q);
    int vals[3] = { 10, 20, 30 };
    for (int i = 0; i < 3; i++) {
        queue_enqueue(&q, &vals[i]);
        EXPECT_EQ(queue_size(&q), i + 1);
    }
    for (int i = 2; i >= 0; i--) {
        queue_dequeue(&q);
        EXPECT_EQ(queue_size(&q), i);
    }
}

// ---------------------------------------------------------------------------
// Destroy
// ---------------------------------------------------------------------------

TEST(Destroy, DestroyLeavesQueueEmpty) {
    DESCRIBE("after destroy the queue is in the empty state again");
    Queue q;
    queue_init(&q);
    int a = 1, b = 2;
    queue_enqueue(&q, &a);
    queue_enqueue(&q, &b);
    queue_destroy(&q);
    EXPECT_TRUE(queue_is_empty(&q));
    EXPECT_EQ(queue_size(&q), 0);
}
