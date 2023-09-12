
#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    Timestamp();
    // ecplicit用在有参数的构造函数 拒绝隐式转换，控制代码行为，构造一个相应对象，才会调用该构造，或者是显式转换    
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};