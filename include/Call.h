#pragma once
#include <string>

struct Call {
    int id = 0;
    int operatorId = 0;

    std::string clientPhone;
    std::string callDate;
    int duration = 0;
    std::string result;

    std::string status = "Зареєстрований";
    double importance = 5.0;
    double urgency = 5.0;
    double waitingTime = 0.0;
};
