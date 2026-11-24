#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

#include "CallCenterPatterns.h"

class TestObserver final : public ICallObserver {
public:
    int received = 0;
    void onCallEvent(const CallEvent&) override { ++received; }
};

int main()
{
    RegisteredCallState registered;
    AcceptedCallState accepted;
    CompletedCallState completed;
    CancelledCallState cancelled;

    assert(registered.canTransitionTo(CallStatus::Accepted));
    assert(registered.canTransitionTo(CallStatus::Cancelled));
    assert(!registered.canTransitionTo(CallStatus::Completed));
    assert(accepted.canTransitionTo(CallStatus::Completed));
    assert(accepted.canTransitionTo(CallStatus::Cancelled));
    assert(!completed.canTransitionTo(CallStatus::Cancelled));
    assert(!cancelled.canTransitionTo(CallStatus::Accepted));

    const PriorityInput input{8, 6, 30};
    MaximumPriorityStrategy maximum;
    WeightedSumPriorityStrategy weighted;
    NormalizedWeightedPriorityStrategy normalized;

    assert(std::abs(maximum.calculate(input) - 8.0) < 1e-9);
    assert(std::abs(weighted.calculate(input) - 6.8) < 1e-9);
    assert(std::abs(normalized.calculate(input) - 68.0) < 1e-9);

    bool invalidInputCaught = false;
    try {
        (void)maximum.calculate({11, 5, 10});
    } catch (const std::invalid_argument&) {
        invalidInputCaught = true;
    }
    assert(invalidInputCaught);

    CallEventPublisher publisher;
    auto first = std::make_shared<TestObserver>();
    auto second = std::make_shared<TestObserver>();

    const int firstId = publisher.subscribe(first);
    publisher.subscribe(second);

    publisher.notify({CallEventType::Registered, 1, "+380000000000", "test"});
    assert(first->received == 1);
    assert(second->received == 1);

    publisher.unsubscribe(firstId);
    publisher.notify({CallEventType::Completed, 1, "+380000000000", "test"});
    assert(first->received == 1);
    assert(second->received == 2);

    std::cout << "All pattern tests passed.\n";
    return 0;
}
