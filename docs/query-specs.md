# Query Language

## Dot-Path Queries

Basic path:

`School.students.name`

### Array Index

`School.students[2].name`

### Wildcard

`School.students[*].name`

## Filter Queries

Basic:

`GET name FROM School.students`

With WHERE:

`GET name FROM School.students WHERE year > 2`

### Multiple Conditions (With AND):

`GET name FROM School.students WHERE year > 2 AND grade >= 90`

## Supported Comparison Operators

- =
- !=
- <
- <=
- \>
- \>=

## String Values in WHERE

`GET name FROM School.students WHERE name = 'Alice'`

## Invalid Queries

Examples:
- `School..students`
- `School.students[-1]`
- `GET name FROMX School.students`
- `GET name FROM School.students WHEREY grade < 70 AND`

## Current Limitations
