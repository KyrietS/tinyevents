#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tinyevents/tinyevents.hpp>

using namespace tinyevents;
using namespace testing;

struct TestToken : public Test {
    Dispatcher dispatcher{};
};

TEST(TestTokenMove, VerifyTokenPredicates) {
    // Check if Token is movable
    EXPECT_TRUE(std::is_move_constructible_v<Token>);
    EXPECT_TRUE(std::is_move_assignable_v<Token>);

    // Check if Token is NOT copyable
    EXPECT_FALSE(std::is_copy_constructible_v<Token>);
    EXPECT_FALSE(std::is_copy_assignable_v<Token>);
}

TEST_F(TestToken, WhenTokenIsDeletedThenHandleIsRemovedFromDispatcher) {
    const auto handle = dispatcher.listen<int>(nullptr);

    // Inner scope to test RAII behavior
    {
        EXPECT_TRUE(dispatcher.hasListener(handle));
        const Token token{dispatcher, handle};
        EXPECT_EQ(token.handle(), handle);
    }
    EXPECT_FALSE(dispatcher.hasListener(handle));
}

TEST_F(TestToken, WhenTokenIsRemovedManuallyThenHandleIsRemovedFromDispatcher) {
    const auto handle = dispatcher.listen<int>(nullptr);

    Token token{dispatcher, handle};
    EXPECT_TRUE(dispatcher.hasListener(handle));
    token.remove();
    EXPECT_FALSE(dispatcher.hasListener(handle));
    EXPECT_EQ(token.handle(), handle); // Handle should not be modified
}

TEST_F(TestToken, WhenTokenIsMovedConstructedThenHandleIsNotRemovedFromDispatcher) {
    const auto handle = dispatcher.listen<int>(nullptr);

    const auto token1 = new Token(dispatcher, handle);
    const auto token2 = new Token(std::move(*token1));
    delete token1;
    EXPECT_TRUE(dispatcher.hasListener(handle));

    delete token2;
    EXPECT_FALSE(dispatcher.hasListener(handle));
}

TEST_F(TestToken, WhenTokenIsMovedAssignedThenHandleIsNotRemovedFromDispatcher) {
    const auto handle1 = dispatcher.listen<int>(nullptr);
    const auto handle2 = dispatcher.listen<int>(nullptr);

    const auto token1 = new Token(dispatcher, handle1);
    const auto token2 = new Token(dispatcher, handle2);

    *token2 = std::move(*token1);
    delete token1;
    ASSERT_TRUE(dispatcher.hasListener(handle1));
    ASSERT_FALSE(dispatcher.hasListener(handle2));
}
