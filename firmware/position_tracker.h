#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>

namespace linear_garage {
// Independent of ESPHome so the actual firmware logic can be replay-tested.
class PositionTracker {
 public:
  static constexpr int FULL_TRAVEL = 1049;  // provisional: one measured cycle
  static constexpr uint32_t STOP_MS = 1500;
  static constexpr uint32_t SETTLE_MS = 2500;

  void sample(int32_t raw, uint32_t now) {
    const int64_t positive = -static_cast<int64_t>(raw);
    if (!sampled_) {
      sampled_ = true;
      last_motion_ = now;
    } else if (positive != count_) {
      direction_ = positive > count_ ? 1 : -1;
      last_motion_ = now;
      // An old closed report must never re-zero a door that has started opening.
      if (direction_ > 0) {
        pending_close_ = false;
        confirmed_closed_ = false;
      }
    }
    count_ = positive;
    if (calibrated_ && (position_count() < -25 || position_count() > FULL_TRAVEL + 55)) {
      calibrated_ = false;
      confirmed_closed_ = false;
      position_fault_ = true;
    }
  }

  void contact(const char *state, uint32_t now) {
    const int next = std::strcmp(state, "off") == 0 ? 0 :
                     std::strcmp(state, "on") == 0 ? 1 : -1;
    if (next == contact_) return;
    const int previous = contact_;
    contact_ = next;
    contact_since_ = now;
    pending_close_ = next == 0;
    confirmed_closed_ = false;
    // If already calibrated, a reconnect reporting closed far from zero is
    // contradictory. Wait for a genuine open -> closed event instead.
    if (pending_close_ && previous != 1 && calibrated_ &&
        (position_count() < -25 || position_count() > 25)) pending_close_ = false;
  }

  bool tick(uint32_t now) {
    if (!sampled_ || !pending_close_ || contact_ != 0 ||
        uint32_t(now - last_motion_) < SETTLE_MS ||
        uint32_t(now - contact_since_) < SETTLE_MS) return false;
    zero_ = count_;
    calibrated_ = true;
    confirmed_closed_ = true;
    position_fault_ = false;
    pending_close_ = false;
    ++calibrations_;
    return true;
  }

  bool calibrated() const { return calibrated_; }
  bool contact_available() const { return contact_ >= 0; }
  int64_t count() const { return count_; }
  // Last observed encoder direction in this boot, including external controls.
  int last_direction() const { return direction_; }
  uint32_t quiet_ms(uint32_t now) const { return uint32_t(now - last_motion_); }
  bool closed_pending() const { return pending_close_ && contact_ == 0; }
  bool position_fault() const { return position_fault_; }
  int64_t position_count() const { return count_ - zero_; }
  unsigned calibrations() const { return calibrations_; }
  float percent() const {
    if (!calibrated_) return NAN;
    const float p = 100.0f * position_count() / FULL_TRAVEL;
    return p < 0 ? 0 : (p > 100 ? 100 : p);
  }
  const char *motion(uint32_t now) const {
    if (direction_ == 0 || uint32_t(now - last_motion_) >= STOP_MS) return "stopped";
    return direction_ > 0 ? "opening" : "closing";
  }
  const char *state(uint32_t now) const {
    const char *m = motion(now);
    if (std::strcmp(m, "stopped") != 0) return m;
    if (!calibrated_) return "unknown";
    if (confirmed_closed_ && contact_ == 0 && position_count() == 0) return "closed";
    if (percent() >= 99.5f) return "open_estimated";
    return "stopped";
  }

 private:
  int64_t count_{0}, zero_{0};
  uint32_t last_motion_{0}, contact_since_{0};
  unsigned calibrations_{0};
  int contact_{-1}, direction_{0};
  bool sampled_{false}, calibrated_{false}, pending_close_{false}, confirmed_closed_{false};
  bool position_fault_{false};
};
}  // namespace linear_garage
