#include "performance_clock.h"

CPerformanceClock::CPerformanceClock() {
    QueryPerformanceFrequency((LARGE_INTEGER*)&this->m_iFreq);
}

void CPerformanceClock::Restart() {
    int64_t iStart;
    QueryPerformanceCounter((LARGE_INTEGER*)&iStart);
    this->m_iStart = (double)iStart / (double)this->m_iFreq * 1000.0;
}

double CPerformanceClock::Elapse() {
    int64_t iEnd;
    QueryPerformanceCounter((LARGE_INTEGER*)&iEnd);
    return (double)iEnd / (double)this->m_iFreq * 1000.0 - this->m_iStart;
}
