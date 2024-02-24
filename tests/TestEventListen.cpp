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

    const auto handle = dispatcher.listen(callback.AsStdFunction());
    EXPECT_TRUE(dispatcher.hasListener(handle));
}

TEST_F(TestEventListen, ReturnedHandlesShouldBeDifferent) {
    StrictMock<MockFunction<void(const int &)>> callback;

    const auto handle1 = dispatcher.listen(callback.AsStdFunction());
    const auto handle2 = dispatcher.listen(callback.AsStdFunction());

    EXPECT_NE(handle1, handle2);
    EXPECT_NE(handle1.value(), handle2.value());
}

TEST_F(TestEventListen, ReturnedHandleShouldBeInvalidAfterRemoval) {
    StrictMock<MockFunction<void(const int &)>> callback;

    const auto handle = dispatcher.listen(callback.AsStdFunction());
    dispatcher.remove(handle);
    EXPECT_FALSE(dispatcher.hasListener(handle));
}

TEST_F(TestEventListen, RemovedListenerShouldNotBeCalled) {
    StrictMock<MockFunction<void(const int &)>> callback;

    const auto handle = dispatcher.listen(callback.AsStdFunction());
    dispatcher.remove(handle);

    EXPECT_CALL(callback, Call(_)).Times(0);
    dispatcher.dispatch(111);
}

TEST_F(TestEventListen, ListenersCanAddAnotherListener) {
    StrictMock<MockFunction<void(const int &)>> callback1;
    StrictMock<MockFunction<void(const int &)>> callback2;

    dispatcher.listen<int>([&](const int &value) {
        callback1.Call(value);
        dispatcher.listen(callback2.AsStdFunction());
    });

    EXPECT_CALL(callback1, Call(111)).Times(1);
    EXPECT_CALL(callback2, Call(111)).Times(0); // Listener added but not called (yet)
    dispatcher.dispatch(111);

    EXPECT_CALL(callback1, Call(222)).Times(1);
    EXPECT_CALL(callback2, Call(222)).Times(1); // Listener added and called
    dispatcher.dispatch(222);

    EXPECT_CALL(callback1, Call(333)).Times(1);
    EXPECT_CALL(callback2, Call(333)).Times(2); // Listener added and called twice
    dispatcher.dispatch(333);
}

TEST_F(TestEventListen, ListenerCanRemoveItself) {
    StrictMock<MockFunction<void(const int &)>> callback;

    ListenerHandle handleToSelf{0};

    handleToSelf = dispatcher.listen<int>([&](const int &value) {
        callback.Call(value);
        dispatcher.remove(handleToSelf);
    });

    EXPECT_CALL(callback, Call(_)).Times(1);
    dispatcher.dispatch(111);

    EXPECT_CALL(callback, Call(_)).Times(0);
    dispatcher.dispatch(222);
}

TEST_F(TestEventListen, ListenerCanRemoveAnotherListener) {
    StrictMock<MockFunction<void(const int &)>> callback1;
    StrictMock<MockFunction<void(const int &)>> callback2;

    ListenerHandle handle2{0};

    dispatcher.listen<int>([&](const int &value) {
        callback1.Call(value);
        dispatcher.remove(handle2);
    });
    handle2 = dispatcher.listen<int>(callback2.AsStdFunction());

    EXPECT_CALL(callback1, Call(_)).Times(1);
    EXPECT_CALL(callback2, Call(_)).Times(0);
    dispatcher.dispatch(111);
}

TEST_F(TestEventListen, ListenerOnceShouldBeRemovedAfterCall) {
    StrictMock<MockFunction<void(const int &)>> callback;

    const auto handle = dispatcher.listenOnce(callback.AsStdFunction());

    EXPECT_CALL(callback, Call(_)).Times(1);

    dispatcher.dispatch(111);
    EXPECT_FALSE(dispatcher.hasListener(handle));
    dispatcher.dispatch(222);
}

TEST_F(TestEventListen, ListenOnceCanBeCalledFromInsideAnotherListenOnceCallback) {
    StrictMock<MockFunction<void(const int &)>> callback1;
    StrictMock<MockFunction<void(const int &)>> callback2;

    dispatcher.listenOnce<int>([this, &callback1, &callback2](const int &value) {
        callback1.Call(value);
        dispatcher.listenOnce<int>([&callback2](const int &value2) {
            callback2.Call(value2);
        });
    });

    EXPECT_CALL(callback1, Call(111)).Times(1);
    EXPECT_CALL(callback2, Call(111)).Times(0);
    dispatcher.dispatch(111);

    EXPECT_CALL(callback1, Call(222)).Times(0);
    EXPECT_CALL(callback2, Call(222)).Times(1);
    dispatcher.dispatch(222);
    dispatcher.dispatch(333);
}

TEST_F(TestEventListen, ListenerOnceShouldBeRemovedAfterCallEvenIfItRemovesItself) {
    StrictMock<MockFunction<void(const int &)>> callback;

    ListenerHandle handleToSelf{0};

    handleToSelf = dispatcher.listenOnce<int>([&](const int &value) {
        callback.Call(value);
        dispatcher.remove(handleToSelf);
        EXPECT_FALSE(dispatcher.hasListener(handleToSelf));
    });

    EXPECT_CALL(callback, Call(_)).Times(1);
    dispatcher.dispatch(111);
    EXPECT_FALSE(dispatcher.hasListener(handleToSelf));
    dispatcher.dispatch(222);
}

TEST_F(TestEventListen, ListenerOnceShouldBeCalledOnceEvenIfMessageIsSentFromListener) {
    StrictMock<MockFunction<void(const int &)>> callback;

    const auto handle = dispatcher.listenOnce<int>([&](const int &value) {
        callback.Call(value);
        dispatcher.dispatch(222);
    });

    EXPECT_CALL(callback, Call(111)).Times(1);
    dispatcher.dispatch(111);
    EXPECT_FALSE(dispatcher.hasListener(handle));
    dispatcher.dispatch(333);
}