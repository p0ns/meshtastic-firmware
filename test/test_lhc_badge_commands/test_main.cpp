#include "modules/LHCBadgeCommands.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_argument_aliases_accept_optional_slash() {
  size_t offset = 0;

  TEST_ASSERT_TRUE(lhc_badge::matchShortCommand("e 12", 'e', &offset));
  TEST_ASSERT_EQUAL_UINT32(1, offset);
  TEST_ASSERT_TRUE(lhc_badge::matchShortCommand("/e 12", 'e', &offset));
  TEST_ASSERT_EQUAL_UINT32(2, offset);
  TEST_ASSERT_TRUE(lhc_badge::matchShortCommand("/b 50", 'b', &offset));
  TEST_ASSERT_TRUE(lhc_badge::matchShortCommand("/s 200", 's', &offset));
  TEST_ASSERT_TRUE(lhc_badge::matchShortCommand("/c 1 2 3", 'c', &offset));
}

void test_no_argument_aliases_accept_optional_slash() {
  TEST_ASSERT_TRUE(lhc_badge::isShortCommand("h", 'h'));
  TEST_ASSERT_TRUE(lhc_badge::isShortCommand("/h", 'h'));
  TEST_ASSERT_TRUE(lhc_badge::isShortCommand("/d", 'd'));
  TEST_ASSERT_TRUE(lhc_badge::isShortCommand("/n", 'n'));
  TEST_ASSERT_TRUE(lhc_badge::isShortCommand("/p", 'p'));
}

void test_aliases_require_a_token_boundary() {
  TEST_ASSERT_FALSE(lhc_badge::matchShortCommand("effect 12", 'e'));
  TEST_ASSERT_FALSE(lhc_badge::matchShortCommand("/effect 12", 'e'));
  TEST_ASSERT_FALSE(lhc_badge::matchShortCommand("/e12", 'e'));
  TEST_ASSERT_FALSE(lhc_badge::isShortCommand("/next", 'n'));
  TEST_ASSERT_FALSE(lhc_badge::isShortCommand("/n now", 'n'));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_argument_aliases_accept_optional_slash);
  RUN_TEST(test_no_argument_aliases_accept_optional_slash);
  RUN_TEST(test_aliases_require_a_token_boundary);
  return UNITY_END();
}
