#pragma once

#include <compare>
#include <cmath>

namespace geom {

struct Vec2D {
    Vec2D() = default;
    Vec2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Vec2D& operator*=(double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    auto operator<=>(const Vec2D&) const = default;

    double x = 0;
    double y = 0;
};

inline Vec2D operator*(Vec2D lhs, double rhs) {
    return lhs *= rhs;
}

inline Vec2D operator*(double lhs, Vec2D rhs) {
    return rhs *= lhs;
}

struct Point2D {
    Point2D() = default;
    Point2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Point2D& operator+=(const Vec2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    //auto operator<=>(const Point2D&) const = default;
    bool operator==(const Point2D& other) const {
        constexpr float EPSILON = 1e-5;
        return std::abs(x - other.x) < EPSILON && std::abs(y - other.y) < EPSILON;
    }

    bool operator!=(const Point2D& other) const {
        return !(*this == other);
    }
    static float Distance(const Point2D& a, const Point2D& b) {
        double squared_distance = pow(b.x - a.x, 2) + pow(b.y - a.y, 2);
        return sqrt(squared_distance);
    }

    double x = 0;
    double y = 0;
};

inline Point2D operator+(Point2D lhs, const Vec2D& rhs) {
    return lhs += rhs;
}

inline Point2D operator+(const Vec2D& lhs, Point2D rhs) {
    return rhs += lhs;
}

}  // namespace geom
