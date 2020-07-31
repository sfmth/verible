// Copyright 2018 The diff-match-patch Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "third_party/verible/external/editscript.h"

#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/str_join.h"

namespace diff {
namespace {

// Define some serialization functions for ease of reading any differences.
std::ostream &operator<<(std::ostream &out, Operation operation) {
  switch (operation) {
    case Operation::EQUALS:
      return (out << "EQUALS");
    case Operation::DELETE:
      return (out << "DELETE");
    case Operation::INSERT:
      return (out << "INSERT");
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, const Edits &edits) {
  out << "Edits{";
  std::string outer_delim = "";
  for (auto &edit : edits) {
    out << outer_delim << "{" << edit.operation
        << ",[" << edit.start << "," << edit.end << ")}";
    outer_delim = ",";
  }
  out << "};";
  return out;
}

std::string ToString(const Edits &edits) {
  std::ostringstream oss;
  oss << edits;
  return oss.str();
}

TEST(DiffTest, CheckEmptyIntVectorDiffResults) {
  std::vector<int> tokens1, tokens2;

  Edits actual = GetTokenDiffs(tokens1.cbegin(), tokens1.cend(),
                               tokens2.cbegin(), tokens2.cend());
  Edits expect = decltype(actual){};
  EXPECT_EQ(ToString(actual), ToString(expect));
}

TEST(DiffTest, CheckEmptyNoCommonSubsequence) {
  const char tokens1[] = "@$&~|";
  const char tokens2[] = "the quick brown fox jumped over the lazy dog";

  Edits actual = GetTokenDiffs(tokens1, tokens1 + strlen(tokens1), tokens2,
                               tokens2 + strlen(tokens2));
  auto expect =
      Edits{{Operation::DELETE, 0, 5},    // = tokens1[ 0,  5) = all of tokens1
            {Operation::INSERT, 0, 44}};  // = tokens2[ 0, 44) = all of tokens2
  EXPECT_EQ(ToString(actual), ToString(expect));

  // Find the longest common subsequence.
  auto max_elem_iter = std::max_element(
      actual.cbegin(), actual.cend(), [](const Edit &lhs, const Edit &rhs) {
        return (lhs.operation == Operation::EQUALS &&
                rhs.operation == Operation::EQUALS &&
                (lhs.end - lhs.start) < (rhs.end - rhs.start));
      });
  ASSERT_NE(0, std::distance(max_elem_iter, actual.cend()));
}

TEST(DiffTest, CheckCharArrayDiffResultsAndLongestCommonSubsequence) {
  const char tokens1[] = "the fox jumped over the dog.";
  const char tokens2[] = "the quick brown fox jumped the lazy dog";

  Edits actual = GetTokenDiffs(tokens1, tokens1 + strlen(tokens1), tokens2,
                               tokens2 + strlen(tokens2));

  // EQUALS and DELETE offsets point into tokens1, INSERT into tokens2.
  auto expect =
      Edits{{Operation::EQUALS, 0, 4},     // = tokens1[ 0,  4) = "the "
            {Operation::INSERT, 4, 16},    // = tokens2[ 4, 16)= "quick brown "
            {Operation::EQUALS, 4, 15},    // = tokens1[ 4, 15) = "fox jumped "
            {Operation::DELETE, 15, 20},   // = tokens1[15, 20) = "over "
            {Operation::EQUALS, 20, 24},   // = tokens1[20, 24) = "the "
            {Operation::INSERT, 31, 36},   // = tokens2[31, 36) = "lazy "
            {Operation::EQUALS, 24, 27},   // = tokens1[24, 27) = "dog"
            {Operation::DELETE, 27, 28}};  // = tokens1[27, 28) = "."
  EXPECT_EQ(ToString(actual), ToString(expect));

  // Find the longest common subsequence.
  auto max_elem_iter = std::max_element(
      actual.cbegin(), actual.cend(), [](const Edit &lhs, const Edit &rhs) {
        return (lhs.operation == Operation::EQUALS &&
                rhs.operation == Operation::EQUALS &&
                (lhs.end - lhs.start) < (rhs.end - rhs.start));
      });
  ASSERT_NE(0, std::distance(max_elem_iter, actual.cend()));
  ASSERT_EQ(Operation::EQUALS, max_elem_iter->operation);
  EXPECT_EQ(4, max_elem_iter->start);
  EXPECT_EQ(15, max_elem_iter->end);
  const std::string longest_common_subsequence(&tokens1[max_elem_iter->start],
                                               &tokens1[max_elem_iter->end]);
  EXPECT_EQ("fox jumped ", longest_common_subsequence);
}

TEST(DiffTest, CheckNonConstStringVectorDiffResults) {
  auto tokens1 = std::vector<std::string>{"the", "fox", "jumped", "over",
                                          "the", "dog", "."};
  auto tokens2 = std::vector<std::string>{"the",    "quick", "brown", "fox",
                                          "jumped", "the",   "lazy",  "dog"};

  // Test use of non-const iterators
  Edits actual = GetTokenDiffs(tokens1.begin(), tokens1.end(), tokens2.begin(),
                               tokens2.end());

  // EQUALS and DELETE offsets point into tokens1, INSERT into tokens2.
  Edits expect = decltype(actual){
      {Operation::EQUALS, 0, 1},   // = tokens1[0, 1) = {"the"}
      {Operation::INSERT, 1, 3},   // = tokens2[1, 3) = {"quick", "brown"}
      {Operation::EQUALS, 1, 3},   // = tokens1[1, 3) = {"fox", "jumped"}
      {Operation::DELETE, 3, 4},   // = tokens1[3, 4) = {"over"}
      {Operation::EQUALS, 4, 5},   // = tokens1[4, 5) = {"the"}
      {Operation::INSERT, 6, 7},   // = tokens2[6, 7) = {"lazy"}
      {Operation::EQUALS, 5, 6},   // = tokens1[5, 6) = {"dog"}
      {Operation::DELETE, 6, 7}};  // = tokens1[6, 7) = {"."}
  EXPECT_EQ(ToString(actual), ToString(expect));

  // Find the longest common subsequence.
  auto max_elem_iter = std::max_element(
      actual.cbegin(), actual.cend(), [](const Edit &lhs, const Edit &rhs) {
        return (lhs.operation == Operation::EQUALS &&
                rhs.operation == Operation::EQUALS &&
                (lhs.end - lhs.start) < (rhs.end - rhs.start));
      });
  ASSERT_NE(0, std::distance(max_elem_iter, actual.cend()));
  ASSERT_EQ(Operation::EQUALS, max_elem_iter->operation);
  EXPECT_EQ(1, max_elem_iter->start);
  EXPECT_EQ(3, max_elem_iter->end);
  const std::string longest_common_subsequence = absl::StrJoin(
      &tokens1[max_elem_iter->start], &tokens1[max_elem_iter->end], " ");
  EXPECT_EQ("fox jumped", longest_common_subsequence);
}

}  // namespace
}  // namespace diff
