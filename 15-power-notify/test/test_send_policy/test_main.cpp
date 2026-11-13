#include <gtest/gtest.h>

#include "SendPolicy.h"

namespace {
constexpr uint32_t kBaseMs = 5000;
constexpr uint32_t kMaxMs = 120000;
}  // namespace

TEST(ClassifySend, SuccessIsSent) {
    EXPECT_EQ(classifySend(200), SendResult::Sent);
}

TEST(ClassifySend, NoConnectionIsRetry) {
    EXPECT_EQ(classifySend(-1), SendResult::Retry);   // DNS / TCP / TLS
    EXPECT_EQ(classifySend(-11), SendResult::Retry);  // read timeout
}

TEST(ClassifySend, RateLimitAndServerErrorsAreRetry) {
    EXPECT_EQ(classifySend(429), SendResult::Retry);
    EXPECT_EQ(classifySend(500), SendResult::Retry);
    EXPECT_EQ(classifySend(502), SendResult::Retry);
}

TEST(ClassifySend, ClientErrorsAreGiveUp) {
    EXPECT_EQ(classifySend(400), SendResult::GiveUp);  // chat not found
    EXPECT_EQ(classifySend(401), SendResult::GiveUp);  // bad token
    EXPECT_EQ(classifySend(404), SendResult::GiveUp);  // malformed token
}

TEST(Backoff, FirstAttemptIsImmediate) {
    const Backoff backoff(kBaseMs, kMaxMs);
    EXPECT_TRUE(backoff.isDue(0));
}

TEST(Backoff, WaitsBaseAfterFirstFailure) {
    Backoff backoff(kBaseMs, kMaxMs);
    backoff.onFailure(1000);
    EXPECT_FALSE(backoff.isDue(1000 + kBaseMs - 1));
    EXPECT_TRUE(backoff.isDue(1000 + kBaseMs));
}

TEST(Backoff, DoublesUpToMax) {
    Backoff backoff(kBaseMs, kMaxMs);
    const uint32_t expected[] = {5000, 10000, 20000, 40000, 80000, 120000, 120000};
    uint32_t now = 0;
    for (const uint32_t delay : expected) {
        backoff.onFailure(now);
        EXPECT_EQ(backoff.delayMs(), delay);
        now += delay;
    }
}

TEST(Backoff, WorksAcrossMillisOverflow) {
    constexpr uint32_t start = 0xFFFFFFFF - 1000;
    Backoff backoff(kBaseMs, kMaxMs);
    backoff.onFailure(start);
    EXPECT_FALSE(backoff.isDue(start + kBaseMs - 1));
    EXPECT_TRUE(backoff.isDue(start + kBaseMs));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
