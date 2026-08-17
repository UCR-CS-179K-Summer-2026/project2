# Query Language Reference

This document tells you exactly how to write a valid query for this system, what's required vs. optional, and what happens when things go wrong. It's written for someone using the system, not reading its source code.

There are two kinds of queries: **dot-path queries** (retrieve a value at a location) and **filter queries** (GET/FROM/WHERE — scan an array and select matching rows).

---

## 1. Dot-Path Queries

A dot-path query is a sequence of **segments** separated by dots (`.`). Each segment is one of:

| Segment type | Syntax | Example | Meaning |
|---|---|---|---|
| Bare key | `identifier` (letters, digits, underscore) | `name` | Look up an object key by plain name |
| Array index | `[N]` | `[0]` | Get the Nth element of an array (0-indexed) |
| Wildcard | `[*]` | `[*]` | Get every element of an array |
| Quoted key | `["key text"]` | `["product name"]` | Look up an object key by exact text — required when the key contains spaces, is empty, or is a lone dot |

**A full path chains these together with dots**, e.g.:
```
store.products[0].name
store.products[*].name
```

### Rule: every segment needs a dot before it — including bracket segments

This is the rule most likely to trip you up. A bracket segment (index, wildcard, *or* quoted key) still needs a `.` before it if it comes after another key, **unless** it's directly attached to a bare key as an array/wildcard access (the one exception, kept for backward compatibility with plain array indexing).

| Query | Valid? | Why |
|---|---|---|
| `items[0].sku` | ✅ | Array index directly after a bare key — the one exception |
| `items[0].["product name"]` | ✅ | Quoted key after an array index — needs the dot |
| `items[0]["product name"]` | ❌ | **Missing dot** — throws `Expected '.' at position N` |
| `["."]` | ✅ | Quoted key as the very first segment — no dot needed since there's nothing before it |
| `data.["."]` | ✅ | Quoted key after a bare key — needs the dot |

**In short: if you're not sure, add the dot.** The only place you can skip it is a bare key immediately followed by `[N]` or `[*]`.

### Quoted keys — when you need them

Use `["..."]` instead of a bare key whenever the JSON key:
- contains a space (`"product name"`)
- is the empty string (`""`)
- is a literal dot (`"."`)
- contains any character a bare identifier can't (anything other than letters, digits, underscore)

Example: for `{"product name": "Laptop"}`, you must write `["product name"]` — `product name` (unquoted) will fail, because the parser stops reading a bare key at the first space it sees.

---

## 2. Filter Queries (GET / FROM / WHERE)

```
GET <path> FROM <path> [WHERE <condition> [AND <condition> ...]]
```

- **GET** — the field to return from each matching row
- **FROM** — the path to the array to scan. **This must resolve to a JSON array.** If it resolves to anything else (an object, a missing key, a scalar), the query returns an **empty result set** — it does not throw an error.
- **WHERE** — optional. If omitted entirely, every row in the array is returned (no filtering happens).

### WHERE conditions

```
WHERE <path> <operator> <value>
```

Supported operators: `=`  `!=`  `<`  `<=`  `>`  `>=`

Multiple conditions are combined with **AND** only — there is no OR:
```
WHERE price > 300 AND inStock = true
```
Every condition must be true for a row to be included.

### Values in WHERE

- A value with no spaces can be written plain: `WHERE category = electronics`
- A value **with spaces must be wrapped in single quotes**: `WHERE name = 'Wireless Mouse'`
- Numbers are compared numerically if the source field is a JSON number; otherwise comparison is a plain string/lexical comparison (this also covers booleans, e.g. `WHERE inStock = true`)

### Quoted keys work in GET, FROM, and WHERE too

The same `["..."]` syntax from dot-path queries works in every clause of a filter query, since all three parse the same way internally:
```
GET ["product name"] FROM store.products WHERE inStock = true
GET price FROM store.products WHERE ["product name"] = Desk
```

---

## 3. What Happens When Things Go Wrong

The system distinguishes between a query that's **malformed** (a syntax problem) and one that's **well-formed but doesn't match anything** (a data problem). These behave very differently — one throws, the other doesn't.

| Situation | Behavior |
|---|---|
| Malformed query syntax (typo, unclosed bracket, trailing dot, etc.) | **Throws an exception** with a message describing the problem |
| A key doesn't exist | Returns `"DNE"` (Does Not Exist) — not an error |
| An array index is out of range | Returns `"DNE"` |
| Wildcard `[*]` applied to something that isn't an array | Returns `"DNE"` |
| Wildcard `[*]` applied to an empty array | Returns **zero results** — not an error, not DNE, just nothing to iterate |
| The actual JSON value at a location is a real `null` | Returns the string `"null"` — deliberately different from `"DNE"`, since these mean different things (a real null value vs. nothing found there) |
| FROM resolves to a non-array | Returns **zero results** — does not throw |
| A negative array index (`[-1]`) | **Throws** — negative indices are rejected at parse time |
| An unquoted WHERE value containing a space | **Throws** — e.g. `WHERE name = Wireless Mouse` fails because only `Wireless` is read as the value and `Mouse` is left over as unrecognized trailing input. Use `WHERE name = 'Wireless Mouse'` instead. |

---

## 4. Complete Examples

```
# Plain dot-path lookup
store.name

# Array index
store.products[0].name

# Wildcard over an array
store.products[*].name

# Quoted key for a key with a space
store.products[0].["product name"]

# Quoted key on a wildcarded array
store.products[*].["product name"]

# Filter query, no WHERE (returns every row)
GET name FROM store.products

# Filter query with one condition
GET name FROM store.products WHERE price > 300

# Filter query with multiple conditions
GET name FROM store.products WHERE category = electronics AND inStock = true

# Filter query with a quoted-space value
GET price FROM store.products WHERE name = 'Wireless Mouse'

# Filter query with a quoted key
GET ["product name"] FROM store.products WHERE inStock = true
```
