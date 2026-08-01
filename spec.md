## DOT PATH QUERIES

accepted syntax:

- path:
- "."
- "[*]" - all elements

examples:

- employees.name
- employees[0].salary
- student.name

## FILTER QUERIES

accepted syntax:
- GET
- FROM
- WHERE
- AND

operators:
- =
- !=
- <
- <=
- \>
- \>=



usage examples:
- GET field (ex: name) FROM path (ex: employees) WHERE condiiton
