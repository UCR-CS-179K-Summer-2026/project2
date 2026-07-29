**Team Aaron, Poojan And Tasmin**

System Architecture:
<img width="544" height="440" alt="SystemArtchitecture" src="https://github.com/user-attachments/assets/5df4a41f-1088-4574-86ec-d0e483bb0496" />

Module relations:

  1.The JSON Parser reads the input file once and produces an in-memory representation of the data.
  2. The Indexer consumes that representation and builds lookup structures so the Query Executor doesn't have to re-scan the whole document per query.
  3. The Query Parser independently parses the query string (dot-path or GET...FROM...WHERE syntax) into a small AST (Abstract Syntax Tree).
  4.The Query Executor takes the parsed query (AST) and the index, and produces the final result.

This separation means parsing the JSON and parsing the query are decoupled, so we can optimize each independently, and we can run many queries against one parsed/indexed file without re-parsing the JSON each time.


Module Descriptions:

JSON Parser:
Responsibility: Convert raw JSON/JSONL text into an internal tree/array representation.
Key design decision: parse once, reuse across multiple queries, rather than re-parsing per query.

Indexer:
Responsibility: Build a structure that maps paths/keys to their locations in the parsed data, so lookups don't require re-traversing the full tree.
Key design decision: trade a one-time indexing cost for much faster repeated queries — this is where a lot of our optimization work will happen (Sprint 2).

Query Parser:
Responsibility: Parse the two supported query forms (dot-path with [*], and GET...FROM...WHERE) into a simple AST the executor can walk.

Query Executor:
Responsibility: Walk the AST against the index to resolve the query and return results in the format defined in our query spec.

Sprint Plan
Sprint 1: Determine parsing and query algorithms; implement first working versions of both dot-path and filter queries on a small dataset. 
Sprint 2: Scale algorithms to larger datasets; optimize search and parsing performance.
Sprint 3: Merge search and parsing into a single working program; source/create a larger dataset for testing.
Sprint 4: Add a user interface (if applicable), thoroughly test the final package on the larger dataset, and finalize for demo.


