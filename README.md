# JSON Analytics Engine

**Team:** Aaron, Poojan, and Tasnim

---

## Summary

This project is a high-performance JSON analytics engine that takes a large JSON file and a query, and returns data that matches the query using dot-path or GET/FROM/WHERE filter queries. The core focus of the project is optimizing the parsing algorithm and the search/query algorithm so that the system stays fast even as input size grows.

### Notable Highlights

- SIMD-accelerated JSON parsing implementation using AVX2
- Dot-path traversal with array and wilcard support
- GET/FROM/WHERE filtering with AND, OR, and NOT conditions
- Query execution optimized for repeated searching over parsed data
  
---

## Quick Start

### 1. Clone Repository

```bash
git clone <repository-url>
cd <repository-name>
```

### 2. Build

### 3. Run Program

### 4. Load JSON File

Enter path or file name when prompted:

```text
Enter JSON file name (or type QUIT / HELP): enter_file_here.json
File loaded successfully.
```

### 5. Enter a Query

After JSON file has loaded, enter query. 

For correct syntax usage, use HELP, and then option 1: Query Types.

```text
Enter a query (or type HELP / QUIT): enter.query
```

After each query, MENU allows different file loading, more query searching, or to QUIT.

## System Requirements

### Software

### Hardware

