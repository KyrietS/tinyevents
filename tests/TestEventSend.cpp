#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;

struct TestEventSend : public Test {
    Dispatcher dispatcher{};
};

TEST_F(TestEventSend, MessagesIgnoredWhenNoListeners) {
    dispatcher.send(123);
    dispatcher.send(123.0f);
    dispatcher.send("abc");

    struct CustomType { };
    dispatcher.send(CustomType{});
}

TEST_F(TestEventSend, MessagesAreDispatchedToSingleListeners) {
    dispatcher.listen<int>([](const auto &msg) {
        EXPECT_EQ(msg, 123);
    });
    dispatcher.send(123);

    int n = 123;
    dispatcher.listen<int>([&n](const auto &msg) {
        EXPECT_EQ(msg, n);
    });
    dispatcher.send(n);
}

TEST_F(TestEventSend, TheSameListenerCanBeAddedMultipleTimes) {
    MockFunction<void(const int &)> intCallback;
    EXPECT_CALL(intCallback, Call(123)).Times(2);

    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.listen(intCallback.AsStdFunction());

    dispatcher.send(123);
}

TEST_F(TestEventSend, DifferentListenersOfSameTypeAreCalled) {
    StrictMock<MockFunction<void(const int &)>> intCallback1;
    StrictMock<MockFunction<void(const int &)>> intCallback2;
    EXPECT_CALL(intCallback1, Call(123)).Times(1);
    EXPECT_CALL(intCallback2, Call(123)).Times(1);

    dispatcher.listen(intCallback1.AsStdFunction());
    dispatcher.listen(intCallback2.AsStdFunction());

    dispatcher.send(123);
}

TEST_F(TestEventSend, DifferentListenersOfDifferentTypesAreCalled) {
    StrictMock<MockFunction<void(const int &)>> intCallback;
    StrictMock<MockFunction<void(const float &)>> floatCallback;


    dispatcher.listen(intCallback.AsStdFunction());
    dispatcher.listen(floatCallback.AsStdFunction());

    EXPECT_CALL(intCallback, Call(1)).Times(1);
    dispatcher.send(1);

    EXPECT_CALL(floatCallback, Call(2.0f)).Times(1);
    dispatcher.send(2.0f);
}


TEST_F(TestEventSend, ListenerWithCustomEmptyType) {
    struct EmptyType { };

    StrictMock<MockFunction<void(const EmptyType &)>> emptyTypeCallback;
    dispatcher.listen(emptyTypeCallback.AsStdFunction());

    EXPECT_CALL(emptyTypeCallback, Call(A<const EmptyType &>())).Times(1);
    dispatcher.send(EmptyType{});

    EXPECT_CALL(emptyTypeCallback, Call(A<const EmptyType &>())).Times(1);
    dispatcher.send(EmptyType{});
}

TEST_F(TestEventSend, ListenerWithCustomType) {
    struct CustomType {
        int n;
    };

    StrictMock<MockFunction<void(const CustomType &)>> customTypeCallback;
    dispatcher.listen(customTypeCallback.AsStdFunction());

    EXPECT_CALL(customTypeCallback, Call(Field(&CustomType::n, 111))).Times(1);
    dispatcher.send(CustomType{ 111 });

    EXPECT_CALL(customTypeCallback, Call(Field(&CustomType::n, 222))).Times(1);
    dispatcher.send(CustomType{ 222 });

    dispatcher.send(333);
}

TEST_F(TestEventSend, ParentMessageTypeListenerShouldNotBeCalledForChildMessage) {
    struct Parent { };
    struct Child : Parent { };

    StrictMock<MockFunction<void(const Parent &)>> parentCallback;
    StrictMock<MockFunction<void(const Child &)>> childCallback;
    dispatcher.listen(parentCallback.AsStdFunction());
    dispatcher.listen(childCallback.AsStdFunction());

    EXPECT_CALL(childCallback, Call(A<const Child &>())).Times(1);
    dispatcher.send(Child{});
}
