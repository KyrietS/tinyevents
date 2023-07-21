#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;

struct TestEventListen : public Test {
    Dispatcher dispatcher{};
};


TEST_F(TestEventListen, VerifyListenerHandlePredicates) {
    // Check if Handle is movable
    EXPECT_TRUE(std::is_move_constructible_v<ListenerHandle>);
    EXPECT_TRUE(std::is_move_assignable_v<ListenerHandle>);

    // Check if Handle is copyable
    EXPECT_TRUE(std::is_copy_constructible_v<ListenerHandle>);
    EXPECT_TRUE(std::is_copy_assignable_v<ListenerHandle>);

    // Check if Handle is comparable
    constexpr bool hasEqualOperator = std::is_same_v<decltype(ListenerHandle{123u} == ListenerHandle{123u}), bool>;
    EXPECT_TRUE(hasEqualOperator);
    constexpr bool hasNotEqualOperator = std::is_same_v<decltype(ListenerHandle{123u} != ListenerHandle{123u}), bool>;
    EXPECT_TRUE(hasNotEqualOperator);
}

TEST_F(TestEventListen, AddingNewListenerShouldReturnHandle) {
    StrictMock<MockFunction<void(const int &)>> callback;

    EXPECT_THAT(dispatcher.listen(callback.AsStdFunction()), A<ListenerHandle>());
}

TEST_F(TestEventListen, ReturnedHandleShouldBeValid) {
    StrictMock<MockFunction<void(const int &)>> callback;

    auto handle = dispatcher.listen(callback.AsStdFunction());
    EXPECT_TRUE(dispatcher.hasListener(handle));
}

TEST_F(TestEventListen, ReturnedHandlesShouldBeDifferent) {
    StrictMock<MockFunction<void(const int &)>> callback;

    auto handle1 = dispatcher.listen(callback.AsStdFunction());
    auto handle2 = dispatcher.listen(callback.AsStdFunction());
    EXPECT_NE(handle1, handle2);
    EXPECT_NE(handle1.value(), handle2.value());
}

TEST_F(TestEventListen, ReturnedHandleShouldBeInvalidAfterRemoval) {
    StrictMock<MockFunction<void(const int &)>> callback;

    auto handle = dispatcher.listen(callback.AsStdFunction());
    dispatcher.remove(handle);
    EXPECT_FALSE(dispatcher.hasListener(handle));
}

TEST_F(TestEventListen, RemovedListenerShouldNotBeCalled) {
    StrictMock<MockFunction<void(const int &)>> callback;

    auto handle = dispatcher.listen(callback.AsStdFunction());
    dispatcher.remove(handle);

    EXPECT_CALL(callback, Call(_)).Times(0);
    dispatcher.send(111);
}
