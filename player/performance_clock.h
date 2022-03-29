#ifndef PERFORMANCE_CLOCK_H
#define PERFORMANCE_CLOCK_H

#include <iostream>
#include <cstdint>
#include <windows.h>

class CPerformanceClock {
public:
    CPerformanceClock();
    ~CPerformanceClock() = default;
    void Restart();
    double Elapse();

private:
    int64_t m_iFreq;     // 频率
    double m_iStart;     // 开始时间
};

#endif // PERFORMANCE_CLOCK_H