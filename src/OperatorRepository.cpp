#include "OperatorRepository.h"

OperatorRepository::OperatorRepository(
    decltype(createStorage("callcenter.db"))& db)
    : storage(&db)
{
}

int OperatorRepository::add(const Operator& item)
{
    return static_cast<int>(storage->insert(item));
}

std::optional<Operator> OperatorRepository::findById(int id) const
{
    return storage->get_optional<Operator>(id);
}

std::vector<Operator> OperatorRepository::findAll() const
{
    return storage->get_all<Operator>();
}

void OperatorRepository::update(const Operator& item)
{
    storage->update(item);
}

void OperatorRepository::remove(int id)
{
    storage->remove<Operator>(id);
}