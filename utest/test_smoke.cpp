#include <gtest/gtest.h>
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, read_sensor, int);

TEST(Smoke, FffFakeReturnsConfiguredValue) {
    RESET_FAKE(read_sensor);
    read_sensor_fake.return_val = 42;
    EXPECT_EQ(read_sensor(1), 42);
    EXPECT_EQ(read_sensor_fake.call_count, 1u);
}
