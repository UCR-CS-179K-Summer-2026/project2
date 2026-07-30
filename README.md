# JSON Analytic Engine

**Team:** Aaron, Poojan, and Tasnim

---

## Summary

This project is a high-performance JSON analytics engine, developed as Option 2 of the course project. Given a large JSON file and a query, the engine returns the data that matches the query. The core focus of the project is optimizing the parsing algorithm and the search/query algorithm so that the system stays fast even as input size grows.

* **Input:** A large JSON file + a query
* **Output:** Data that answers the given query
* **Language:** C++
* **Goal:** Parse and query JSON data with low latency, minimum time and memory overhead even on larger datasets.

---

## Supported Query Types (for Sprint 1)

We plan to support two query styles:

### 1. Dot-Path Queries (with wildcard support)

It uses dot notation to specify a path into the JSON structure. `[*]` iterates over all elements of an array and returns one result per element, in array order.

#### Rules:
* If a key used in a query is missing from an element inside a `[*]` array, that element's result is `null` (not skipped), and output length always matches array length.
* Results preserve duplicates.

#### Example JSON:
```json
{
  "Google": {
    "employees": [
      {
        "employeeId": 1000,
        "name": "John Doe",
        "jobTitle": "Software Engineer",
        "department": "Engineering",
        "salary": 100000
      },
      {
        "employeeId": 1001,
        "name": "Jane Doe",
        "jobTitle": "Cloud Engineer",
        "department": "Engineering",
        "salary": 90000
      },
      {
        "employeeId": 1002,
        "name": "Johnny Doe",
        "jobTitle": "Database Engineer",
        "department": "Engineering",
        "salary": 75000
      },
      {
        "employeeId": 3
      }
    ]
  }
}
```

#### Test Cases:

| Query | Result |
| :--- | :--- |
| `Google.employees[*].name` | `["John Doe", "Jane Doe", "Johnny Doe", null]` |
| `Google.employees[*].employeeId` | `[1000, 1001, 1002, 3]` |
| `Google.employees[*].jobTitle` | `["Software Engineer", "Cloud Engineer", "Database Engineer", null]` |
| `Google.employees[*].salary` | `[100000, 90000, 75000, null]` |
| `Google.employees[*].department` | `["Engineering", "Engineering", "Engineering", null]` |
| `Google.employees[*].email` | `[null, null, null, null]` |

---

### 2. SQL-style Filter Queries (`GET ..., FROM ..., WHERE ...`)

Selects a field from records in an array that match a condition. Supports comparison operators (`=`, `<`, `>`, etc.) and logical `AND`.

**Rule:** If a record is missing a field referenced in the `WHERE` clause, that record is excluded from the result (it cannot satisfy the condition).

#### Test Cases:

| Query | Result |
| :--- | :--- |
| `GET name FROM Google.employees WHERE employeeId = 1001` | `["Jane Doe"]` |
| `GET employeeId FROM Google.employees WHERE salary < 80000` | `[1002]` |
| `GET name FROM Google.employees WHERE department = "Engineering" AND salary > 80000` | `["John Doe", "Jane Doe"]` |

---

## Tech Stack

* **Language:** C++
* **Focus Areas:** Parsing algorithm optimization, query/search algorithm optimization, data locality, and cache aware design.

---
