#pragma once

#include <raylib.h>

struct RuntimeInputState {
    bool toggleInfoMenu{false};
    bool toggleInterestZones{false};
    float zoomWheelDelta{0.0f};
    float moveX{0.0f};
    float moveY{0.0f};
    bool sprinting{false};
    int facingDirection{0};
};

class RuntimeInputManager {
public:
    RuntimeInputState poll(bool infoMenuOpen) const;
};
