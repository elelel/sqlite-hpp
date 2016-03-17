#include <gtest/gtest.h>

#define SQLITE_HPP_LOG_FILENAME "sqlite_debug.log"

#include <sqlite>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <random>
#include <limits>

TEST(SqliteTest, OpenDb) {
  sqlite::database db("test.db");
  ASSERT_EQ(db.result_code(), SQLITE_OK);
}

TEST(SqliteTest, CStyleQuery) {
  // Simple sqlite maniplation, mimics sqlite's native C interface
  sqlite::database::type_ptr db(new sqlite::database::type("test.db"));
  ASSERT_EQ(SQLITE_OK, db->result_code());
  // Create the query. Sqlite's prepare() will be called automatically
  // if object is constructed with a SQL query string
  sqlite::query drop_table(db, "DROP TABLE IF EXISTS `test_table`");
  // Call step() function to execute the query
  drop_table.step();
  ASSERT_EQ(SQLITE_DONE, drop_table.result_code());
  sqlite::query create_table(db, "CREATE TABLE `test_table` \
(`str_field` TEXT, \
`blob_field` BLOB, \
`int_field` INTEGER, \
`float_field` FLOAT)");
  create_table.step();
  ASSERT_EQ(SQLITE_DONE, create_table.result_code());
  sqlite::query insert(db, "INSERT INTO `test_table` \
(`str_field`, `blob_field`, `int_field`, `float_field`) \
VALUES (?, ?, ?, ?)");
  // Bind the sql parameters. That is the usual sqlite parameter binding.
  // Parameter index start with 1
  insert.bind(1, std::string("12345"));
  insert.bind(2, std::vector<uint8_t>{1,2,3,4,5});
  insert.bind(3, 12345);
  insert.bind(4, 1.2345);
  insert.step();
  ASSERT_EQ(SQLITE_DONE, insert.result_code());
  sqlite::query select(db, "SELECT \
`str_field`, `blob_field`, `int_field`, `float_field` \
FROM `test_table`");
  // Call step() function to execute the query, should be called for
  // each row if we expect select to return more than one row
  select.step();
  // Get the first (0th) element of current row
  std::string text_value = select.get<std::string>(0);
  std::vector<uint8_t> blob_value;
  // 'Get' can be called as a procedure with output argument
  select.get(1, blob_value);
  int integer_value = select.get<int>(2);
  double float_value = select.get<double>(3);
  ASSERT_EQ(text_value, "12345");
  ASSERT_EQ(blob_value, std::vector<uint8_t>({1,2,3,4,5}));
  ASSERT_EQ(integer_value, 12345);
  ASSERT_EQ(float_value, 1.2345);
}

TEST(SqliteTest, ComplexBatchQuery) {
  // Advanced sqlite manipulation with query buffering and STL-style iteration
  
  // Create random test source data
  std::uniform_real_distribution<double> uniform(0, 1);
  std::default_random_engine re;
  std::vector<double> source_data;
  // Create a lot of records that would cause an insert lag if inserted without buffering, in separate queries
  for (int i = 0; i < 10000; ++i) {
    source_data.push_back(uniform(re));
  }

  // Define our record type. Records of this type will be returned by our select query
  typedef std::tuple<std::string, std::vector<uint8_t>, int, double> record_type;
  // Create lambda function to deterministically generate bogus data from random double
  auto derive_bogus_record = [] (double d) {
    // Create some data by scaling of the input double number
    int n = std::round(d * double(100.0));
    std::vector<uint8_t> blob;
    for (uint8_t i = 1; i < n; ++i) {
      uint8_t k(std::numeric_limits<uint8_t>::max() / (i * n));
      blob.push_back(k);
    } 
    return record_type(std::to_string(d), blob, n, d);
  };

  // Create expected data. These are the data we are expecting to get back
  // after storing the transformed original source data in sqlite.

  std::vector<record_type> expected_data;
  std::transform(source_data.begin(), source_data.end(), std::back_inserter(expected_data), derive_bogus_record);

  // Create database to store the data
  sqlite::database::type_ptr db(new sqlite::database::type("test.db"));
  ASSERT_EQ(SQLITE_OK, db->result_code());
  sqlite::query drop_table(db, "DROP TABLE IF EXISTS `test_table`");
  drop_table.step();
  ASSERT_EQ(SQLITE_DONE, drop_table.result_code());
  sqlite::query create_table(db, "CREATE TABLE `test_table` \
(`id` INTEGER PRIMARY KEY AUTOINCREMENT, \
`str_field` TEXT, \
`blob_field` BLOB, \
`int_field` INTEGER, \
`float_field` FLOAT)");
  create_table.step();
  ASSERT_EQ(SQLITE_DONE, create_table.result_code());

  // Create buffered insert query.
  typedef sqlite::buffered_insert_query<std::string, std::vector<uint8_t>, int, double> insert_type;
  insert_type insert(db, "test_table", std::vector<std::string>{"str_field", "blob_field", "int_field", "float_field"});
  // Insert data into sql while transforming it, exactly as we did when generated expected data randomly.
  std::transform(source_data.begin(), source_data.end(), std::back_inserter(insert), derive_bogus_record);
  ASSERT_EQ(SQLITE_DONE, insert.result_code());
  // flush() has to be called, as we may still have buffered data in buffered insert query.
  // It is called automatically on object destruction, so generally you won't need to do this
  // manually if you follow RIIA pattern in your program.
  insert.flush();
  ASSERT_EQ(SQLITE_DONE, insert.result_code());

  // Create select query to get the data back from sqlite by creating STL-compliant input iterator.
  typedef sqlite::input_query<std::string, std::vector<uint8_t>, int, double> select_type;
  select_type select(db, "SELECT `str_field`, `blob_field`, `int_field`, `float_field` FROM `test_table` ORDER BY `id`");
  auto expected_row = expected_data.begin();
  // Iterate the result of select query as if it were an STL container.
  for (auto row : select) {
    ASSERT_EQ(*expected_row, row);
    ++expected_row;
  }
}