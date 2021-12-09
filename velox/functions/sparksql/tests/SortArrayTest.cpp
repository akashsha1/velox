/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <limits>
#include <optional>

#include "velox/exec/tests/utils/FunctionUtils.h"
#include "velox/functions/prestosql/tests/FunctionBaseTest.h"
#include "velox/functions/sparksql/Register.h"
#include "velox/functions/sparksql/tests/SparkFunctionBaseTest.h"
#include "velox/vector/ComplexVector.h"

namespace facebook::velox::functions::sparksql::test {
namespace {

using facebook::velox::functions::test::FunctionBaseTest;

class SortArrayTest : public SparkFunctionBaseTest {
 protected:
  template <typename T>
  void testSortArray(
      const VectorPtr& input,
      std::vector<std::vector<std::optional<T>>>& expected) {
    // Verify that by default array is sorted in ascending order.
    auto result =
        evaluate<ArrayVector>("sort_array(c0)", makeRowVector({input}));
    assertEqualVectors(makeNullableArrayVector(expected), result);

    // Verify sort order with asc flag set to true.
    auto resultAsc =
        evaluate<ArrayVector>("sort_array(c0, true)", makeRowVector({input}));
    assertEqualVectors(makeNullableArrayVector(expected), resultAsc);

    // Verify sort order with asc flag set to false.
    auto resultDesc =
        evaluate<ArrayVector>("sort_array(c0, false)", makeRowVector({input}));
    for (auto& v : expected) {
      std::reverse(v.begin(), v.end());
    }
    assertEqualVectors(makeNullableArrayVector(expected), resultDesc);
  }

  template <typename T>
  void testInt() {
    auto min = std::numeric_limits<T>::min();
    auto max = std::numeric_limits<T>::max();
    auto input = makeNullableArrayVector<T>({
        {},
        {9, 8, 12},
        {5, 6, 1, std::nullopt, 0, 99, -99},
        {std::nullopt, std::nullopt},
        {min, max, -1, 1, 0, std::nullopt},
    });
    std::vector<std::vector<std::optional<T>>> expected = {
        {},
        {8, 9, 12},
        {std::nullopt, -99, 0, 1, 5, 6, 99},
        {std::nullopt, std::nullopt},
        {std::nullopt, min, -1, 0, 1, max},
    };
    testSortArray<T>(input, expected);
  }

  template <typename T>
  void testFloatingPoint() {
    auto lowest = std::numeric_limits<T>::lowest();
    auto max = std::numeric_limits<T>::max();
    auto inf = std::numeric_limits<T>::infinity();
    auto nan = std::numeric_limits<T>::quiet_NaN();
    auto input = makeNullableArrayVector<T>({
        {},
        {1.0001, std::nullopt, 1.0, -2.0, 3.03, std::nullopt},
        {std::nullopt, std::nullopt},
        {max, lowest, nan, inf, -9.009, 9.009, std::nullopt, 0.0},
    });
    std::vector<std::vector<std::optional<T>>> expected = {
        {},
        {std::nullopt, std::nullopt, -2.0, 1.0, 1.0001, 3.03},
        {std::nullopt, std::nullopt},
        {std::nullopt, lowest, -9.009, 0.0, 9.009, max, inf, nan},
    };
    testSortArray<T>(input, expected);
  }
};

TEST_F(SortArrayTest, invalidInput) {
  auto arg0 = makeNullableArrayVector<int>({{0, 1}});
  std::vector<bool> v = {false};
  auto arg1 = makeFlatVector<bool>(v);
  ASSERT_THROW(
      evaluate<ArrayVector>("sort_array(c0, c1)", makeRowVector({arg0, arg1})),
      VeloxException);
}

TEST_F(SortArrayTest, int8) {
  testInt<int8_t>();
}

TEST_F(SortArrayTest, int16) {
  testInt<int16_t>();
}

TEST_F(SortArrayTest, int32) {
  testInt<int32_t>();
}

TEST_F(SortArrayTest, int64) {
  testInt<int64_t>();
}

TEST_F(SortArrayTest, float) {
  testFloatingPoint<float>();
}

TEST_F(SortArrayTest, double) {
  testFloatingPoint<double>();
}

TEST_F(SortArrayTest, string) {
  auto input = makeNullableArrayVector<std::string>({
      {},
      {"spiderman", "captainamerica", "ironman", "hulk", "deadpool", "thor"},
      {"s", "c", "", std::nullopt, "h", "d"},
      {std::nullopt, std::nullopt},
  });
  std::vector<std::vector<std::optional<std::string>>> expected = {
      {},
      {"captainamerica", "deadpool", "hulk", "ironman", "spiderman", "thor"},
      {std::nullopt, "", "c", "d", "h", "s"},
      {std::nullopt, std::nullopt},
  };
  testSortArray<std::string>(input, expected);
}

TEST_F(SortArrayTest, timestamp) {
  using T = Timestamp;
  auto input = makeNullableArrayVector<Timestamp>({
      {},
      {T{0, 1}, T{1, 0}, std::nullopt, T{4, 20}, T{3, 30}},
      {std::nullopt, std::nullopt},
  });
  std::vector<std::vector<std::optional<Timestamp>>> expected = {
      {},
      {std::nullopt, T{0, 1}, T{1, 0}, T{3, 30}, T{4, 20}},
      {std::nullopt, std::nullopt},
  };
  testSortArray<Timestamp>(input, expected);
}

TEST_F(SortArrayTest, date) {
  using D = Date;
  auto input = makeNullableArrayVector<Date>({
      {},
      {D{0}, D{1}, std::nullopt, D{4}, D{3}},
      {std::nullopt, std::nullopt},
  });
  std::vector<std::vector<std::optional<Date>>> expected = {
      {},
      {std::nullopt, D{0}, D{1}, D{3}, D{4}},
      {std::nullopt, std::nullopt},
  };
  testSortArray<Date>(input, expected);
}

} // namespace
} // namespace facebook::velox::functions::sparksql::test
