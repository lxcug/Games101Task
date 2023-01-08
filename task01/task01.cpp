#include <iostream>
#include "Eigen/Core"
#include <cmath>


Eigen::Vector2f twoDimAffineTransformation(const Eigen::Vector2f &point) {
    // 将p转化为齐次坐标
    Eigen::Vector3f homoPoint = {point.x(), point.y(),1.f};

    // rotate 45°
    Eigen::Matrix3f rot45;
    rot45 << (float)cos(std::numbers::pi/4), -(float)sin(std::numbers::pi/4), 0,
            (float)sin(std::numbers::pi/4), (float)cos(std::numbers::pi/4), 0,
            0, 0, 1;
    homoPoint = rot45 * homoPoint;

    // translation
    Eigen::Matrix3f translation;
    translation << 1, 0, 1,
            0, 1, 2,
            0, 0, 1;

    homoPoint = translation * homoPoint;
    return {homoPoint.x(), homoPoint.y()};
}


int main() {
    Eigen::Vector2f P(2.f, 1.f);
    Eigen::Vector2f pPrime = twoDimAffineTransformation(P);
    std::cout << pPrime << std::endl;
    return 0;
}