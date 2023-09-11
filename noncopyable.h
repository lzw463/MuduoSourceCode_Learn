#pragma once


/*
noncopyable被继承以后，派生类对象可以正常的进行构造和析构，但是无法进行拷贝构造和赋值操作
*/

class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
    
protected:
    noncopyable() = default;  //派生类对象可以访问
    ~noncopyable() = default;
};
