#pragma once
#include <string>

struct CallStatusHistory {
    int id = 0;
    int callId = 0;
    std::string oldStatus;
    std::string newStatus;
    std::string changedAt;
};
