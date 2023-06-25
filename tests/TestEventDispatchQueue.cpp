#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;

struct DispatcherQueueTest : public testing::Test {
    Dispatcher dispatcher{};
};

TEST_F(DispatcherQueueTest, MessagesQueuedAndThenIgnoredWhenNoListeners) {
    dispatcher.queue(123);
    dispatcher.queue(123.0f);
    dispatcher.queue("abc");

    struct CustomType { };
    dispatcher.queue(CustomType{});

    dispatcher.process();
}

TEST_F(DispatcherQueueTest, ShouldNotSendAnythingWhenNoMessagesQueued) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.process();
}

TEST_F(DispatcherQueueTest, ShouldSendMessageQueued) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.queue(123);
    dispatcher.queue(123);
    dispatcher.queue(123);

    EXPECT_CALL(intCallback, Call(123)).Times(3);
    dispatcher.process();
}

TEST_F(DispatcherQueueTest, ShouldSendQueuedMessagesOfDifferentTypes) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.queue(123);
    dispatcher.queue(123.0f);
    dispatcher.queue("123");

    EXPECT_CALL(intCallback, Call(123)).Times(1);
    dispatcher.process();
}

TEST_F(DispatcherQueueTest, MessageQeuedBeforeListenerAddedAndThenSent) {
    StrictMock<MockFunction<void(const int &)>> intCallback;

    dispatcher.queue(123);
    dispatcher.listen(intCallback.AsStdFunction());

    EXPECT_CALL(intCallback, Call(123)).Times(1);
    dispatcher.process();
}
