// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "arrow/array.h"
#include "arrow/column.h"
#include "arrow/schema.h"
#include "arrow/status.h"
#include "arrow/table.h"
#include "arrow/test-util.h"
#include "arrow/type.h"

using std::shared_ptr;
using std::vector;

namespace arrow {

class TestTable : public TestBase {
 public:
  void MakeExample1(int length) {
    auto f0 = std::make_shared<Field>("f0", int32());
    auto f1 = std::make_shared<Field>("f1", uint8());
    auto f2 = std::make_shared<Field>("f2", int16());

    vector<shared_ptr<Field>> fields = {f0, f1, f2};
    schema_ = std::make_shared<Schema>(fields);

    columns_ = {
        std::make_shared<Column>(schema_->field(0), MakePrimitive<Int32Array>(length)),
        std::make_shared<Column>(schema_->field(1), MakePrimitive<UInt8Array>(length)),
        std::make_shared<Column>(schema_->field(2), MakePrimitive<Int16Array>(length))};
  }

 protected:
  std::shared_ptr<Table> table_;
  shared_ptr<Schema> schema_;
  vector<std::shared_ptr<Column>> columns_;
};

TEST_F(TestTable, EmptySchema) {
  auto empty_schema = shared_ptr<Schema>(new Schema({}));
  table_.reset(new Table("data", empty_schema, columns_));
  ASSERT_OK(table_->ValidateColumns());
  ASSERT_EQ(0, table_->num_rows());
  ASSERT_EQ(0, table_->num_columns());
}

TEST_F(TestTable, Ctors) {
  int length = 100;
  MakeExample1(length);

  std::string name = "data";

  table_.reset(new Table(name, schema_, columns_));
  ASSERT_OK(table_->ValidateColumns());
  ASSERT_EQ(name, table_->name());
  ASSERT_EQ(length, table_->num_rows());
  ASSERT_EQ(3, table_->num_columns());

  table_.reset(new Table(name, schema_, columns_, length));
  ASSERT_OK(table_->ValidateColumns());
  ASSERT_EQ(name, table_->name());
  ASSERT_EQ(length, table_->num_rows());
}

TEST_F(TestTable, Metadata) {
  int length = 100;
  MakeExample1(length);

  std::string name = "data";
  table_.reset(new Table(name, schema_, columns_));

  ASSERT_TRUE(table_->schema()->Equals(schema_));

  auto col = table_->column(0);
  ASSERT_EQ(schema_->field(0)->name, col->name());
  ASSERT_EQ(schema_->field(0)->type, col->type());
}

TEST_F(TestTable, InvalidColumns) {
  // Check that columns are all the same length
  int length = 100;
  MakeExample1(length);

  table_.reset(new Table("data", schema_, columns_, length - 1));
  ASSERT_RAISES(Invalid, table_->ValidateColumns());

  columns_.clear();

  // Wrong number of columns
  table_.reset(new Table("data", schema_, columns_, length));
  ASSERT_RAISES(Invalid, table_->ValidateColumns());

  columns_ = {
      std::make_shared<Column>(schema_->field(0), MakePrimitive<Int32Array>(length)),
      std::make_shared<Column>(schema_->field(1), MakePrimitive<UInt8Array>(length)),
      std::make_shared<Column>(schema_->field(2), MakePrimitive<Int16Array>(length - 1))};

  table_.reset(new Table("data", schema_, columns_, length));
  ASSERT_RAISES(Invalid, table_->ValidateColumns());
}

TEST_F(TestTable, Equals) {
  int length = 100;
  MakeExample1(length);

  std::string name = "data";
  table_.reset(new Table(name, schema_, columns_));

  ASSERT_TRUE(table_->Equals(table_));
  ASSERT_FALSE(table_->Equals(nullptr));
  // Differing name
  ASSERT_FALSE(table_->Equals(std::make_shared<Table>("other_name", schema_, columns_)));
  // Differing schema
  auto f0 = std::make_shared<Field>("f3", int32());
  auto f1 = std::make_shared<Field>("f4", uint8());
  auto f2 = std::make_shared<Field>("f5", int16());
  vector<shared_ptr<Field>> fields = {f0, f1, f2};
  auto other_schema = std::make_shared<Schema>(fields);
  ASSERT_FALSE(table_->Equals(std::make_shared<Table>(name, other_schema, columns_)));
  // Differing columns
  std::vector<std::shared_ptr<Column>> other_columns = {
      std::make_shared<Column>(schema_->field(0), MakePrimitive<Int32Array>(length, 10)),
      std::make_shared<Column>(schema_->field(1), MakePrimitive<UInt8Array>(length, 10)),
      std::make_shared<Column>(schema_->field(2), MakePrimitive<Int16Array>(length, 10))};
  ASSERT_FALSE(table_->Equals(std::make_shared<Table>(name, schema_, other_columns)));
}

class TestRecordBatch : public TestBase {};

TEST_F(TestRecordBatch, Equals) {
  const int length = 10;

  auto f0 = std::make_shared<Field>("f0", int32());
  auto f1 = std::make_shared<Field>("f1", uint8());
  auto f2 = std::make_shared<Field>("f2", int16());

  vector<shared_ptr<Field>> fields = {f0, f1, f2};
  auto schema = std::make_shared<Schema>(fields);

  auto a0 = MakePrimitive<Int32Array>(length);
  auto a1 = MakePrimitive<UInt8Array>(length);
  auto a2 = MakePrimitive<Int16Array>(length);

  RecordBatch b1(schema, length, {a0, a1, a2});
  RecordBatch b2(schema, 5, {a0, a1, a2});
  RecordBatch b3(schema, length, {a0, a1});
  RecordBatch b4(schema, length, {a0, a1, a1});

  ASSERT_TRUE(b1.Equals(b1));
  ASSERT_FALSE(b1.Equals(b2));
  ASSERT_FALSE(b1.Equals(b3));
  ASSERT_FALSE(b1.Equals(b4));
}

}  // namespace arrow
