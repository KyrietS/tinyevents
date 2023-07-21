#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;

struct TestEventDispatch : public Test {
    Dispatcher dispatcher{};
};

TEST_F(TestEventDispatch, MessagesIgnoredWhenNoListeners) {
    dispatcher.dispatch(123);
    dispatcher.dispatch(123.0f);
    dispatcher.dispatch("abc");

    struct CustomType { };
    dispatcher.dispatch(CustomType{});
}

TEST_F(TestEventDispatch, MessagesAreDispatchedToSingleListeners) {
    dispatcher.listen<int>([](const auto &msg) {
        EXPECT_EQ(msg, 123);
    });
    dispatcher.dispatch(123);

    int n = 123;
    dispatcher.listen<int>([&n](const auto &msg) {
        EXPECT_EQ(msg, n);
    });
    dispatcher.dispatch(n);
}

TEST_F(TestEventDispatch, TheSameListenerCanBeAddedMultipleTimes) {
    MockFunction<void(const int &)> intCallback;
    EXPECT_CALL(intCallback, Call(123)).Times(2);

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.listen(intCallback.AsStdFunction());

    dispatcher.dispatch(123);
}

TEST_F(TestEventDispatch, ListenerCanDispatchEventToItself)
{
    StrictMock<MockFunction<void(const int &)>> callback;

    dispatcher.listen<int>([&](const int &value) {
        callback.Call(value);
        if (value == 111)
            dispatcher.dispatch(999);
    });

    EXPECT_CALL(callback, Call(111)).Times(1);
    EXPECT_CALL(callback, Call(999)).Times(1);
    dispatcher.dispatch(111);
}

TEST_F(TestEventDispatch, DifferentListenersOfSameTypeAreCalled) {
    StrictMock<MockFunction<void(const int &)>> intCallback1;
    StrictMock<MockFunction<void(const int &)>> intCallback2;
    EXPECT_CALL(intCallback1, Call(123)).Times(1);
    EXPECT_CALL(intCallback2, Call(123)).Times(1);

    dispatcher.listen(intCallback1.AsStdFunction());
    dispatcher.listen(intCallback2.AsStdFunction());

    dispatcher.dispatch(123);
}

TEST_F(TestEventDispatch, DifferentListenersOfDifferentTypesAreCalled) {
    StrictMock<MockFunction<void(const int &)>> intCallback;
    StrictMock<MockFunction<void(const float &)>> floatCallback;


    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.listen(floatCallback.AsStdFunction());

    EXPECT_CALL(intCallback, Call(1)).Times(1);
    dispatcher.dispatch(1);

    EXPECT_CALL(floatCallback, Call(2.0f)).Times(1);
    dispatcher.dispatch(2.0f);
}


TEST_F(TestEventDispatch, ListenerWithCustomEmptyType) {
    struct EmptyType { };

    StrictMock<MockFunction<void(const EmptyType &)>> emptyTypeCallback;
    dispatcher.listen(emptyTypeCallback.AsStdFunction());

    EXPECT_CALL(emptyTypeCallback, Call(A<const EmptyType &>())).Times(1);
    dispatcher.dispatch(EmptyType{});

    EXPECT_CALL(emptyTypeCallback, Call(A<const EmptyType &>())).Times(1);
    dispatcher.dispatch(EmptyType{});
}

TEST_F(TestEventDispatch, ListenerWithCustomType) {
    struct CustomType {
        int n;
    };

    StrictMock<MockFunction<void(const CustomType &)>> customTypeCallback;
    dispatcher.listen(customTypeCallback.AsStdFunction());

    EXPECT_CALL(customTypeCallback, Call(Field(&CustomType::n, 111))).Times(1);
    dispatcher.dispatch(CustomType{ 111 });

    EXPECT_CALL(customTypeCallback, Call(Field(&CustomType::n, 222))).Times(1);
    dispatcher.dispatch(CustomType{ 222 });

    dispatcher.dispatch(333);
}

TEST_F(TestEventDispatch, ParentMessageTypeListenerShouldNotBeCalledForChildMessage) {
    struct Parent { };
    struct Child : Parent { };

    StrictMock<MockFunction<void(const Parent &)>> parentCallback;
    StrictMock<MockFunction<void(const Child &)>> childCallback;
    dispatcher.listen(parentCallback.AsStdFunction());
    dispatcher.listen(childCallback.AsStdFunction());

    EXPECT_CALL(childCallback, Call(A<const Child &>())).Times(1);
    dispatcher.dispatch(Child{});
}
