#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;


TEST(TestDispatcherCopyAndMove, VerifyDispatcherPredicates) {
    // Check if Dispatcher is movable
    EXPECT_TRUE(std::is_move_constructible_v<Dispatcher>);
    EXPECT_TRUE(std::is_move_assignable_v<Dispatcher>);

    // Check if Dispatcher is NOT copyable
    EXPECT_FALSE(std::is_copy_constructible_v<Dispatcher>);
    EXPECT_FALSE(std::is_copy_assignable_v<Dispatcher>);
}

TEST(TestDispatcherCopyAndMove, WhenDispatcherIsMovedThenListenersAreMoved) {
    Dispatcher movedDispatcher;
    StrictMock<MockFunction<void(const int &)>> intCallback;

    movedDispatcher.listen(intCallback.AsStdFunction());
    Dispatcher dispatcher = std::move(movedDispatcher);

    // Should not call the listener
    EXPECT_CALL(intCallback, Call(222)).Times(0);
    movedDispatcher.send(222); // NOLINT(*-use-after-move)

    EXPECT_CALL(intCallback, Call(111)).Times(1);
    dispatcher.send(111);
}

TEST(TestDispatcherCopyAndMove, WhenDispatcherIsMovedThenQueuedMessagesAreMoved) {
    Dispatcher movedDispatcher;
    StrictMock<MockFunction<void(const int &)>> intCallback;

    movedDispatcher.listen(intCallback.AsStdFunction());
    movedDispatcher.queue(111);
    Dispatcher dispatcher = std::move(movedDispatcher);

    // Should not call the listener
    EXPECT_CALL(intCallback, Call(222)).Times(0);
    movedDispatcher.process(); // NOLINT(*-use-after-move)

    EXPECT_CALL(intCallback, Call(111)).Times(1);
    dispatcher.process();
}