#include "drake/common/yaml/yaml_write_archive.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "drake/common/name_value.h"
#include "drake/common/test_utilities/expect_throws_message.h"
#include "drake/common/yaml/test/example_structs.h"

namespace drake {
namespace yaml {
namespace test {
namespace {

// A test fixture with common helpers.
class YamlWriteArchiveTest : public ::testing::Test {
 public:
  template <typename Serializable>
  static std::string Save(const Serializable& data) {
    YamlWriteArchive archive;
    archive.Accept(data);
    return archive.EmitString("doc");
  }

  static std::string WrapDoc(const std::string& value) {
    return "doc:\n  value: " + value + "\n";
  }
};

TEST_F(YamlWriteArchiveTest, Double) {
  const auto test = [](const double value, const std::string& expected) {
    const DoubleStruct x{value};
    EXPECT_EQ(Save(x), WrapDoc(expected));
  };

  test(0.0, "0.0");
  test(1.0, "1.0");
  test(-1.0, "-1.0");

  test(0.009, "0.009");

  test(1.2, "1.2");
  test(-1.2, "-1.2");

  test(5.6e+16, "5.6e+16");
  test(5.6e-12, "5.6e-12");
  test(-5.6e+16, "-5.6e+16");
  test(-5.6e-12, "-5.6e-12");
}

TEST_F(YamlWriteArchiveTest, String) {
  const auto test = [](const std::string& value, const std::string& expected) {
    const StringStruct x{value};
    EXPECT_EQ(Save(x), WrapDoc(expected));
  };

  test("a", "a");
  test("1", "1");
}

TEST_F(YamlWriteArchiveTest, StdArray) {
  const auto test = [](const std::array<double, 3>& value,
                       const std::string& expected) {
    const ArrayStruct x{value};
    EXPECT_EQ(Save(x), expected);
  };

  test({1.0, 2.0, 3.0}, R"R(doc:
  value: [1.0, 2.0, 3.0]
)R");
}

TEST_F(YamlWriteArchiveTest, StdVector) {
  const auto test = [](const std::vector<double>& value,
                       const std::string& expected) {
    const VectorStruct x{value};
    EXPECT_EQ(Save(x), expected);
  };

  // When the vector items are simple YAML scalars, we should use
  // "flow" style, where they all appear on a single line.
  test({}, R"R(doc:
  value: []
)R");

  test({1.0, 2.0, 3.0}, R"R(doc:
  value: [1.0, 2.0, 3.0]
)R");

  // When the vector items are not simple scalars, we should use
  // "block" style, where each gets its own line(s).
  NonPodVectorStruct x;
  x.value = {
    {.value = "foo"},
    {.value = "bar"},
  };
  EXPECT_EQ(Save(x), R"R(doc:
  value:
    - value: foo
    - value: bar
)R");
}

TEST_F(YamlWriteArchiveTest, StdMap) {
  const auto test = [](const std::map<std::string, double>& value,
                       const std::string& expected) {
    const MapStruct x{value};
    EXPECT_EQ(Save(x), expected);
  };

  test({}, R"R(doc:
  value: {}
)R");

  test({{"foo", 0.0}}, R"R(doc:
  value:
    foo: 0.0
)R");

  test({{"foo", 0.0}, {"bar", 1.0}}, R"R(doc:
  value:
    bar: 1.0
    foo: 0.0
)R");
}


TEST_F(YamlWriteArchiveTest, StdUnorderedMap) {
  const auto test = [](const std::unordered_map<std::string, double>& value,
                       const std::string& expected) {
    const UnorderedMapStruct x{value};
    EXPECT_EQ(Save(x), expected);
  };

  // The YamlWriteArchive API promises that the output must be deterministic.
  // Here, we test a stronger condition that it's always sorted.  (If in the
  // future we decide to use a different order, we can update here to match.)
  test({
    // Use many values to increase the chance that we'll detect implementations
    // that fail to sort.
    {"gulf", 6.0},
    {"fox", 5.0},
    {"echo", 4.0},
    {"delta", 3.0},
    {"charlie", 2.0},
    {"bravo", 1.0},
    {"alpha", 0.0},
  }, R"R(doc:
  value:
    alpha: 0.0
    bravo: 1.0
    charlie: 2.0
    delta: 3.0
    echo: 4.0
    fox: 5.0
    gulf: 6.0
)R");
}

TEST_F(YamlWriteArchiveTest, Optional) {
  const auto test = [](const std::optional<double>& value,
                       const std::string& expected) {
    const OptionalStruct x{value};
    EXPECT_EQ(Save(x), expected);
  };

  test(std::nullopt, "doc:\n");
  test(1.0, "doc:\n  value: 1.0\n");
}

TEST_F(YamlWriteArchiveTest, Variant) {
  const auto test = [](const Variant3& value, const std::string& expected) {
    const VariantStruct x{value};
    EXPECT_EQ(Save(x), WrapDoc(expected));
  };

  test(Variant3(std::string("foo")), "foo");
  test(Variant3(DoubleStruct{1.0}), "!DoubleStruct\n    value: 1.0");

  // TODO(jwnimmer-tri) We'd like to see "!!float 1.0" here, but our writer
  // does not yet support that output syntax.
  DRAKE_EXPECT_THROWS_MESSAGE(
    Save(VariantStruct{double{1.0}}),
    std::exception,
    "Cannot YamlWriteArchive the variant type double with a non-zero index");
}

TEST_F(YamlWriteArchiveTest, EigenVector) {
  const auto test = [](const Eigen::VectorXd& value,
                       const std::string& expected) {
    const EigenVecStruct x{value};
    EXPECT_EQ(Save(x), expected);
    const EigenVec3Struct x3{value};
    EXPECT_EQ(Save(x3), expected);
  };

  test(Eigen::Vector3d(1.0, 2.0, 3.0), R"R(doc:
  value: [1.0, 2.0, 3.0]
)R");
}

TEST_F(YamlWriteArchiveTest, EigenVectorX) {
  const auto test = [](const Eigen::VectorXd& value,
                       const std::string& expected) {
    const EigenVecStruct x{value};
    EXPECT_EQ(Save(x), expected);
  };

  test(Eigen::VectorXd(), R"R(doc:
  value: []
)R");
  test(Eigen::Matrix<double, 1, 1>(1.0), R"R(doc:
  value: [1.0]
)R");
}

TEST_F(YamlWriteArchiveTest, EigenMatrix) {
  using Matrix34d = Eigen::Matrix<double, 3, 4>;
  const auto test = [](const Matrix34d& value, const std::string& expected) {
    const EigenMatrixStruct x{value};
    EXPECT_EQ(Save(x), expected);
    const EigenMatrix34Struct x3{value};
    EXPECT_EQ(Save(x3), expected);
  };

  test((Matrix34d{} << 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11).finished(),
       R"R(doc:
  value:
    - [0.0, 1.0, 2.0, 3.0]
    - [4.0, 5.0, 6.0, 7.0]
    - [8.0, 9.0, 10.0, 11.0]
)R");
}

TEST_F(YamlWriteArchiveTest, Nested) {
  OuterStruct x;
  x.inner_struct.inner_value = 2.0;
  x.outer_value = 1.0;

  const std::string saved = Save(x);
  const std::string expected = R"R(doc:
  outer_value: 1.0
  inner_struct:
    inner_value: 2.0
)R";
  EXPECT_EQ(saved, expected);
}

TEST_F(YamlWriteArchiveTest, BlankInner) {
  OuterWithBlankInner x;
  x.outer_value = 1.0;

  const std::string saved = Save(x);
  const std::string expected = R"R(doc:
  outer_value: 1.0
  inner_struct: {}
)R";
  EXPECT_EQ(saved, expected);
}

}  // namespace
}  // namespace test
}  // namespace yaml
}  // namespace drake
