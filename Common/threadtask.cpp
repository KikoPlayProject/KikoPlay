#include "threadtask.h"

void TaskContext::cancel()
{
    bool expected = false;
    if (_cancelRequested.compare_exchange_strong(expected, true))
    {
        emit cancelRequested();
    }
}
