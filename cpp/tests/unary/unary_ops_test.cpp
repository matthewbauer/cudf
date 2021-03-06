/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.
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

#include <cudf_test/base_fixture.hpp>
#include <cudf_test/column_utilities.hpp>
#include <cudf_test/column_wrapper.hpp>
#include <cudf_test/type_lists.hpp>

#include <cudf/unary.hpp>

template <typename T>
cudf::test::fixed_width_column_wrapper<T> create_fixed_columns(cudf::size_type start,
                                                               cudf::size_type size,
                                                               bool nullable)
{
  auto iter = cudf::test::make_counting_transform_iterator(start, [](auto i) { return T(i); });

  if (not nullable) {
    return cudf::test::fixed_width_column_wrapper<T>(iter, iter + size);
  } else {
    auto valids = cudf::test::make_counting_transform_iterator(
      0, [](auto i) { return i % 2 == 0 ? true : false; });
    return cudf::test::fixed_width_column_wrapper<T>(iter, iter + size, valids);
  }
}

template <typename T>
cudf::test::fixed_width_column_wrapper<T> create_expected_columns(cudf::size_type size,
                                                                  bool nullable,
                                                                  bool nulls_to_be)
{
  if (not nullable) {
    auto iter = cudf::test::make_counting_transform_iterator(
      0, [nulls_to_be](auto i) { return not nulls_to_be; });
    return cudf::test::fixed_width_column_wrapper<T>(iter, iter + size);
  } else {
    auto iter = cudf::test::make_counting_transform_iterator(
      0, [nulls_to_be](auto i) { return i % 2 == 0 ? not nulls_to_be : nulls_to_be; });
    return cudf::test::fixed_width_column_wrapper<T>(iter, iter + size);
  }
}

template <typename T>
struct IsNull : public cudf::test::BaseFixture {
};

TYPED_TEST_CASE(IsNull, cudf::test::NumericTypes);

TYPED_TEST(IsNull, AllValid)
{
  using T = TypeParam;

  cudf::size_type start                         = 0;
  cudf::size_type size                          = 10;
  cudf::test::fixed_width_column_wrapper<T> col = create_fixed_columns<T>(start, size, false);
  cudf::test::fixed_width_column_wrapper<bool> expected =
    create_expected_columns<bool>(size, false, true);

  std::unique_ptr<cudf::column> got = cudf::is_null(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNull, WithInvalids)
{
  using T = TypeParam;

  cudf::size_type start                         = 0;
  cudf::size_type size                          = 10;
  cudf::test::fixed_width_column_wrapper<T> col = create_fixed_columns<T>(start, size, true);
  cudf::test::fixed_width_column_wrapper<bool> expected =
    create_expected_columns<bool>(size, true, true);

  std::unique_ptr<cudf::column> got = cudf::is_null(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNull, EmptyColumns)
{
  using T = TypeParam;

  cudf::size_type start                         = 0;
  cudf::size_type size                          = 0;
  cudf::test::fixed_width_column_wrapper<T> col = create_fixed_columns<T>(start, size, true);
  cudf::test::fixed_width_column_wrapper<bool> expected =
    create_expected_columns<bool>(size, true, true);

  std::unique_ptr<cudf::column> got = cudf::is_null(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

template <typename T>
struct IsNotNull : public cudf::test::BaseFixture {
};

TYPED_TEST_CASE(IsNotNull, cudf::test::NumericTypes);

TYPED_TEST(IsNotNull, AllValid)
{
  using T = TypeParam;

  cudf::size_type start                         = 0;
  cudf::size_type size                          = 10;
  cudf::test::fixed_width_column_wrapper<T> col = create_fixed_columns<T>(start, size, false);
  cudf::test::fixed_width_column_wrapper<bool> expected =
    create_expected_columns<bool>(size, false, false);

  std::unique_ptr<cudf::column> got = cudf::is_valid(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNotNull, WithInvalids)
{
  using T = TypeParam;

  cudf::size_type start                         = 0;
  cudf::size_type size                          = 10;
  cudf::test::fixed_width_column_wrapper<T> col = create_fixed_columns<T>(start, size, true);
  cudf::test::fixed_width_column_wrapper<bool> expected =
    create_expected_columns<bool>(size, true, false);

  std::unique_ptr<cudf::column> got = cudf::is_valid(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNotNull, EmptyColumns)
{
  using T = TypeParam;

  cudf::size_type start                         = 0;
  cudf::size_type size                          = 0;
  cudf::test::fixed_width_column_wrapper<T> col = create_fixed_columns<T>(start, size, true);
  cudf::test::fixed_width_column_wrapper<bool> expected =
    create_expected_columns<bool>(size, true, false);

  std::unique_ptr<cudf::column> got = cudf::is_valid(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

template <typename T>
struct IsNAN : public cudf::test::BaseFixture {
};

TYPED_TEST_CASE(IsNAN, cudf::test::FloatingPointTypes);

TYPED_TEST(IsNAN, AllValid)
{
  using T = TypeParam;

  cudf::test::fixed_width_column_wrapper<T> col{{T(1), T(2), T(NAN), T(4), T(NAN), T(6), T(7)}};
  cudf::test::fixed_width_column_wrapper<bool> expected = {
    false, false, true, false, true, false, false};

  std::unique_ptr<cudf::column> got = cudf::is_nan(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNAN, WithNull)
{
  using T = TypeParam;

  // The last NAN is null
  cudf::test::fixed_width_column_wrapper<T> col{{T(1), T(2), T(NAN), T(4), T(NAN), T(6), T(7)},
                                                {1, 0, 1, 1, 0, 1, 1}};
  cudf::test::fixed_width_column_wrapper<bool> expected = {
    false, false, true, false, false, false, false};

  std::unique_ptr<cudf::column> got = cudf::is_nan(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNAN, EmptyColumn)
{
  using T = TypeParam;

  cudf::test::fixed_width_column_wrapper<T> col{};
  cudf::test::fixed_width_column_wrapper<bool> expected = {};

  std::unique_ptr<cudf::column> got = cudf::is_nan(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNAN, NonFloatingColumn)
{
  using T = TypeParam;

  cudf::test::fixed_width_column_wrapper<int32_t> col{{1, 2, 5, 3, 5, 6, 7}, {1, 0, 1, 1, 0, 1, 1}};

  EXPECT_THROW(std::unique_ptr<cudf::column> got = cudf::is_nan(col), cudf::logic_error);
}

template <typename T>
struct IsNotNAN : public cudf::test::BaseFixture {
};

TYPED_TEST_CASE(IsNotNAN, cudf::test::FloatingPointTypes);

TYPED_TEST(IsNotNAN, AllValid)
{
  using T = TypeParam;

  cudf::test::fixed_width_column_wrapper<T> col{{T(1), T(2), T(NAN), T(4), T(NAN), T(6), T(7)}};
  cudf::test::fixed_width_column_wrapper<bool> expected = {
    true, true, false, true, false, true, true};

  std::unique_ptr<cudf::column> got = cudf::is_not_nan(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNotNAN, WithNull)
{
  using T = TypeParam;

  // The last NAN is null
  cudf::test::fixed_width_column_wrapper<T> col{{T(1), T(2), T(NAN), T(4), T(NAN), T(6), T(7)},
                                                {1, 0, 1, 1, 0, 1, 1}};
  cudf::test::fixed_width_column_wrapper<bool> expected = {
    true, true, false, true, true, true, true};

  std::unique_ptr<cudf::column> got = cudf::is_not_nan(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNotNAN, EmptyColumn)
{
  using T = TypeParam;

  cudf::test::fixed_width_column_wrapper<T> col{};
  cudf::test::fixed_width_column_wrapper<bool> expected = {};

  std::unique_ptr<cudf::column> got = cudf::is_not_nan(col);

  CUDF_TEST_EXPECT_COLUMNS_EQUAL(expected, got->view());
}

TYPED_TEST(IsNotNAN, NonFloatingColumn)
{
  using T = TypeParam;

  cudf::test::fixed_width_column_wrapper<int64_t> col{{1, 2, 5, 3, 5, 6, 7}, {1, 0, 1, 1, 0, 1, 1}};

  EXPECT_THROW(std::unique_ptr<cudf::column> got = cudf::is_not_nan(col), cudf::logic_error);
}

CUDF_TEST_PROGRAM_MAIN()
