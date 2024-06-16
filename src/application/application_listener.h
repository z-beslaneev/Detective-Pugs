#pragma once

#include <chrono>

class IApplicationlListener {
public:
    virtual void OnUpdate(const std::chrono::milliseconds& delta) = 0;
    virtual ~IApplicationlListener() = default;
};