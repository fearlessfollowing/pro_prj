/*
 * log测试及属性系统测试
 */
#include <system_properties.h>
#include <log/log_wrapper.h>

#define LOG_TAG "log_test"

#include <log/log.h>

#define TEST_PROP "sys.test_prop"



int main(int argc, char* argv[])
{
    property_set(TEST_PROP, "this is a test");
    LogWrapper::init("/mnt/record/log", "test_log");
    int i = 0;
    while (true) {
        LOGERR(LOG_TAG, "This is a simple test, cnt = %d", i++);
        sleep (1);
    }
    return 0;
}