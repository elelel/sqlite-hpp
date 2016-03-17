# C++ header-only library for SQLITE
## Features
* STL-compliant interface. You can treat queries the way you treat STL containers (std::vectors, etc.). ```for (auto record : select_query) { ... }```
* Interacting in C-style the way you're used to dealing with C SQLITE interface is possible
* Generic approach to type conversion. You are not locked down to returning some fixed type predefined by the library (e.g. vector<T> for BLOBs can easily be replaced by naked char * pointers or even not returned at all and processed immediately). 
* Support for buffered inserts of multiple records. Just feed the data into buffered_insert_query, and it will create as few SQL queries as possible.
* Exception-free C++ code (in a sense that no new exception throwing is introduced by default by the library). Can be useful in embeded or some other restricted environment, or if you're simple not too crazy about C++ exceptions. Result codes of each operation can be retrived by result_code() method and are the native SQLITE C result codes
* Header-only library - no need to compile as separate translation units, just add it to your C++ with native Sqlite

## Requirements
1. Requirements to use the library
  1. C++11-compliant compiler
2. To build the tests (== examples)
  1. Google Test library
  2. CMake

## Adding to a project
If you already have an existing project with Sqlite library installed, just add the library directory to your project. This can be done by copying to your project's source dir (Visual Studio users also need to use "Add existing files.." IDE feature). Another way is to save the library's files to a separate dir and add it to project's includes (that's probably what CMake users would prefer). Then just include "sqlite" header file.
To create a new project, before doing the above add the native Sqlite3 library (you can use the copy from the test dir of this project).

## Using the library
The following snippets are from the tests included with the project. You may want to refer to the sources in test directory for more details
### Simple SQL manipulations mirroring
The library can be used to interact with SQLITE like with the original library's C-style API
Note that at the moment only the functions that I use in my projects are exposed
```c_cpp
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
```
### STL-style interactions
```c_cpp
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
```

### Configuring return types
See value_access_policy.hpp file in include/src

## License
BSD
