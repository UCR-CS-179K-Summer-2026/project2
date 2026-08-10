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

Implemented features:

- case insensitive

## Current Limitations

- we don't support NOT and OR filters yet
- We can't do something like `WHERE (name = '...' AND '...') OR ...`, so no paranthesis. this would go with OR condition when implemented
- we don't have a way for the user to order their results, if they want multiple
