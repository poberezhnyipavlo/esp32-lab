#include "SendPolicy.h"

namespace {
constexpr int kHttpOkFirst = 200;
constexpr int kHttpOkLast = 299;
constexpr int kHttpToManyRequests = 429;
constexpr int kHttpServerErrorFirst = 500;
} // namespace

SendResult classifySend(int http_code) {
    if (http_code < 0) {
        return SendResult::Retry;
    }
    if (http_code >= kHttpOkFirst && http_code <= kHttpOkLast) {
        return SendResult::Sent;
    }
    if (http_code == kHttpToManyRequests || http_code >= kHttpServerErrorFirst) {
        return SendResult::Retry;
    }
    return SendResult::GiveUp;
}

Backoff::Backoff(uint32_t base_ms, uint32_t max_ms)
    : base_ms_(base_ms), max_ms_(max_ms) {  }

bool Backoff::isDue(uint32_t now_ms) const {
    if (delay_ms_ == 0) {
        return true;
    }
    return now_ms - failed_at_ms_ >= delay_ms_;
}

void Backoff::onFailure(uint32_t now_ms) {
    failed_at_ms_ = now_ms;
    if (delay_ms_ == 0) {
        delay_ms_ = base_ms_;
        return;
    }
    delay_ms_ = (delay_ms_ > max_ms_ / 2) ? max_ms_ : delay_ms_ * 2;
}

uint32_t Backoff::delayMs() const {
    return delay_ms_;
}
