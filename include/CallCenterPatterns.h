#pragma once

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Database.h"

enum class CallStatus {
    Registered,
    Accepted,
    Completed,
    Cancelled
};

inline std::string toString(CallStatus status)
{
    switch (status) {
        case CallStatus::Registered: return "Зареєстрований";
        case CallStatus::Accepted:   return "Прийнятий";
        case CallStatus::Completed:  return "Завершений";
        case CallStatus::Cancelled:  return "Скасований";
    }
    throw std::invalid_argument("Unknown call status");
}

inline CallStatus statusFromString(const std::string& value)
{
    if (value == "Зареєстрований") return CallStatus::Registered;
    if (value == "Прийнятий") return CallStatus::Accepted;
    if (value == "Завершений") return CallStatus::Completed;
    if (value == "Скасований") return CallStatus::Cancelled;
    throw std::invalid_argument("Unknown status in database: " + value);
}

class ICallState {
public:
    virtual ~ICallState() = default;
    virtual bool canTransitionTo(CallStatus next) const = 0;
    virtual std::string name() const = 0;
};

class RegisteredCallState final : public ICallState {
public:
    bool canTransitionTo(CallStatus next) const override {
        return next == CallStatus::Accepted || next == CallStatus::Cancelled;
    }
    std::string name() const override { return "Зареєстрований"; }
};

class AcceptedCallState final : public ICallState {
public:
    bool canTransitionTo(CallStatus next) const override {
        return next == CallStatus::Completed || next == CallStatus::Cancelled;
    }
    std::string name() const override { return "Прийнятий"; }
};

class CompletedCallState final : public ICallState {
public:
    bool canTransitionTo(CallStatus) const override { return false; }
    std::string name() const override { return "Завершений"; }
};

class CancelledCallState final : public ICallState {
public:
    bool canTransitionTo(CallStatus) const override { return false; }
    std::string name() const override { return "Скасований"; }
};

inline std::unique_ptr<ICallState> makeCallState(CallStatus status)
{
    switch (status) {
        case CallStatus::Registered: return std::make_unique<RegisteredCallState>();
        case CallStatus::Accepted:   return std::make_unique<AcceptedCallState>();
        case CallStatus::Completed:  return std::make_unique<CompletedCallState>();
        case CallStatus::Cancelled:  return std::make_unique<CancelledCallState>();
    }
    throw std::invalid_argument("Cannot create state");
}

struct PriorityInput {
    double importance = 5.0;
    double urgency = 5.0;
    double waitingTime = 0.0;
};

class ICallPriorityStrategy {
public:
    virtual ~ICallPriorityStrategy() = default;
    virtual double calculate(const PriorityInput& input) const = 0;
    virtual std::string name() const = 0;
};

inline PriorityInput validatePriorityInput(PriorityInput input)
{
    if (input.importance < 1 || input.importance > 10 ||
        input.urgency < 1 || input.urgency > 10 ||
        input.waitingTime < 0) {
        throw std::invalid_argument(
            "Importance and urgency must be 1..10; waiting time must be >= 0");
    }
    input.waitingTime = std::min(input.waitingTime, 60.0);
    return input;
}

class MaximumPriorityStrategy final : public ICallPriorityStrategy {
public:
    double calculate(const PriorityInput& raw) const override {
        const auto p = validatePriorityInput(raw);
        const double normalizedWaiting = p.waitingTime / 6.0; // 0..10
        return std::max({p.importance, p.urgency, normalizedWaiting});
    }
    std::string name() const override { return "Максимум"; }
};

class WeightedSumPriorityStrategy final : public ICallPriorityStrategy {
public:
    double calculate(const PriorityInput& raw) const override {
        const auto p = validatePriorityInput(raw);
        const double normalizedWaiting = p.waitingTime / 6.0; // 0..10
        return 0.5 * p.importance +
               0.3 * p.urgency +
               0.2 * normalizedWaiting;
    }
    std::string name() const override { return "Зважена сума"; }
};

class NormalizedWeightedPriorityStrategy final : public ICallPriorityStrategy {
public:
    double calculate(const PriorityInput& raw) const override {
        const auto p = validatePriorityInput(raw);
        // Нормування кожного параметра до діапазону 0..1.
        const double i = p.importance / 10.0;
        const double u = p.urgency / 10.0;
        const double w = p.waitingTime / 60.0;
        // Результат у відсотковій шкалі 0..100.
        return 100.0 * (0.5 * i + 0.3 * u + 0.2 * w);
    }
    std::string name() const override { return "Нормована зважена сума"; }
};


enum class CallEventType {
    Registered,
    Completed
};

struct CallEvent {
    CallEventType type;
    int callId;
    std::string phone;
    std::string timestamp;
};

class ICallObserver {
public:
    virtual ~ICallObserver() = default;
    virtual void onCallEvent(const CallEvent& event) = 0;
};

class CallEventPublisher {
private:
    std::map<int, std::weak_ptr<ICallObserver>> observers;
    int nextId = 1;

public:
    int subscribe(const std::shared_ptr<ICallObserver>& observer) {
        if (!observer) throw std::invalid_argument("Observer cannot be null");
        const int id = nextId++;
        observers[id] = observer;
        return id;
    }

    void unsubscribe(int id) {
        observers.erase(id);
    }

    void notify(const CallEvent& event) {
        for (auto it = observers.begin(); it != observers.end();) {
            if (auto observer = it->second.lock()) {
                observer->onCallEvent(event);
                ++it;
            } else {
                it = observers.erase(it);
            }
        }
    }
};

class OperatorJournal final : public ICallObserver {
public:
    void onCallEvent(const CallEvent& event) override {
        const std::string eventName =
            event.type == CallEventType::Registered
                ? "реєстрація дзвінка" : "завершення дзвінка";
        std::cout << "[Журнал оператора] " << eventName
                  << ", ID=" << event.callId
                  << ", телефон=" << event.phone
                  << ", час=" << event.timestamp << '\n';
    }
};

class CallStatistics final : public ICallObserver {
private:
    int registeredCount = 0;
    int completedCount = 0;

public:
    void onCallEvent(const CallEvent& event) override {
        if (event.type == CallEventType::Registered) ++registeredCount;
        if (event.type == CallEventType::Completed) ++completedCount;
    }

    int registered() const { return registeredCount; }
    int completed() const { return completedCount; }

    void print() const {
        std::cout << "[Статистика] Зареєстровано: " << registeredCount
                  << ", завершено: " << completedCount << '\n';
    }
};

inline std::string currentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

template <typename Storage>
class CallCenterService {
private:
    Storage& storage;
    CallEventPublisher publisher;
    std::unique_ptr<ICallPriorityStrategy> strategy;

public:
    explicit CallCenterService(Storage& db)
        : storage(db),
          strategy(std::make_unique<WeightedSumPriorityStrategy>())
    {}

    int subscribe(const std::shared_ptr<ICallObserver>& observer) {
        return publisher.subscribe(observer);
    }

    void unsubscribe(int id) {
        publisher.unsubscribe(id);
    }

    void setStrategy(std::unique_ptr<ICallPriorityStrategy> newStrategy) {
        if (!newStrategy) throw std::invalid_argument("Strategy cannot be null");
        strategy = std::move(newStrategy);
    }

    std::string strategyName() const { return strategy->name(); }

    double calculatePriority(const Call& call) const {
        return strategy->calculate(
            {call.importance, call.urgency, call.waitingTime});
    }

    int registerCall(Call call) {
        call.id = 0;
        call.status = toString(CallStatus::Registered);
        const int id = static_cast<int>(storage.insert(call));

        // Подія публікується тільки після успішного запису в БД.
        publisher.notify({CallEventType::Registered, id,
                          call.clientPhone, currentTimestamp()});
        return id;
    }

    void changeStatus(int callId, CallStatus newStatus) {
        Call updated;
        std::string oldStatus;

        // Оновлення дзвінка та історії виконується атомарно.
        const bool committed = storage.transaction([&] {
            auto existing = storage.template get_optional<Call>(callId);
            if (!existing) {
                throw std::runtime_error("Call not found");
            }

            oldStatus = existing->status;
            const CallStatus current = statusFromString(oldStatus);
            auto state = makeCallState(current);

            if (!state->canTransitionTo(newStatus)) {
                throw std::logic_error(
                    "Forbidden status transition: " + oldStatus +
                    " -> " + toString(newStatus));
            }

            updated = *existing;
            updated.status = toString(newStatus);
            storage.update(updated);

            CallStatusHistory history;
            history.callId = callId;
            history.oldStatus = oldStatus;
            history.newStatus = updated.status;
            history.changedAt = currentTimestamp();
            storage.insert(history);

            return true;
        });

        if (!committed) {
            throw std::runtime_error("Transaction was rolled back");
        }

        // Сповіщаємо підписників після COMMIT.
        if (newStatus == CallStatus::Completed) {
            publisher.notify({CallEventType::Completed, callId,
                              updated.clientPhone, currentTimestamp()});
        }
    }

    std::vector<CallStatusHistory> getHistory(int callId) const {
        return storage.template get_all<CallStatusHistory>(
            sqlite_orm::where(
                sqlite_orm::c(&CallStatusHistory::callId) == callId));
    }
};


template <typename Storage>
class CallCenterFacade {
private:
    CallCenterService<Storage>& service;

public:
    explicit CallCenterFacade(CallCenterService<Storage>& s) : service(s) {}

    int registerNewCall(const Call& call) {
        return service.registerCall(call);
    }

    void acceptCall(int id) {
        service.changeStatus(id, CallStatus::Accepted);
    }

    void completeCall(int id) {
        service.changeStatus(id, CallStatus::Completed);
    }

    void cancelCall(int id) {
        service.changeStatus(id, CallStatus::Cancelled);
    }

    double priority(const Call& call) const {
        return service.calculatePriority(call);
    }

    void usePriorityStrategy(std::unique_ptr<ICallPriorityStrategy> s) {
        service.setStrategy(std::move(s));
    }

    int subscribe(const std::shared_ptr<ICallObserver>& observer) {
        return service.subscribe(observer);
    }

    void unsubscribe(int id) {
        service.unsubscribe(id);
    }
};
