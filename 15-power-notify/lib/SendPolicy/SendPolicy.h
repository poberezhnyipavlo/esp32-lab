#pragma once

#include <cstdint>

enum class SendResult {
    Sent,
    Retry,
    GiveUp,
};

SendResult classifySend(int http_code);

class Backoff {
public:
    Backoff(uint32_t base_ms, uint32_t max_ms);

    [[nodiscard]] bool isDue(uint32_t now_ms) const;
    void onFailure(uint32_t now_ms);
    [[nodiscard]] uint32_t delayMs() const;

private:
    uint32_t base_ms_;
    uint32_t max_ms_;
    uint32_t delay_ms_ = 0;
    uint32_t failed_at_ms_ = 0;
};
