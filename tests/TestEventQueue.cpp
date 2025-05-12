#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;

struct TestEventQueue : public testing::Test {
    Dispatcher dispatcher{};
};

TEST_F(TestEventQueue, MessagesQueuedAndThenIgnoredWhenNoListeners) {
    dispatcher.queue(123);
    dispatcher.queue(123.0f);
    dispatcher.queue("abc");

    // Dispatch by rvalue
    struct CustomType { };
    dispatcher.queue(CustomType{});

    // Dispatch by lvalue
    CustomType customType;
    dispatcher.queue(customType);

    dispatcher.process();
}

TEST_F(TestEventQueue, ShouldNotDispatchAnythingWhenNoMessagesQueued) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.process();
}

TEST_F(TestEventQueue, ShouldDispatchMessageQueued) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.queue(123);
    dispatcher.queue(123);
    dispatcher.queue(123);

    EXPECT_CALL(intCallback, Call(123)).Times(3);
    dispatcher.process();
}

TEST_F(TestEventQueue, ShouldDispatchQueuedMessagesOfDifferentTypes) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.queue(123);
    dispatcher.queue(123.0f);
    dispatcher.queue("123");

    EXPECT_CALL(intCallback, Call(123)).Times(1);
    dispatcher.process();
}

TEST_F(TestEventQueue, MessageQeuedBeforeListenerAddedAndThenSent) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.queue(123);
    dispatcher.listen(intCallback.AsStdFunction());

    EXPECT_CALL(intCallback, Call(123)).Times(1);
    dispatcher.process();
}

TEST_F(TestEventQueue, MessageQueuedDataGoesOutOfScope) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    {
        int i = 123;
        dispatcher.queue(i);
    }

    {
        int i = 456;
        dispatcher.queue(i);
    }

    dispatcher.listen(intCallback.AsStdFunction());

    EXPECT_CALL(intCallback, Call(123)).Times(1);
    EXPECT_CALL(intCallback, Call(456)).Times(1);
    dispatcher.process();
}

TEST_F(TestEventQueue, MessageQueueInsideQueuedListener) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen<int>([&](const int &msg) {
        if (msg == 123) {
            dispatcher.queue(456);
        }
    });

    dispatcher.listen(intCallback.AsStdFunction());

    EXPECT_CALL(intCallback, Call(123)).Times(1);
    EXPECT_CALL(intCallback, Call(456)).Times(1);
    dispatcher.queue(123);
    dispatcher.process();
}
