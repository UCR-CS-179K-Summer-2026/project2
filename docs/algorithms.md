## Pseudocode:


### Query Parser Pseudocode:

the main parser:

```text
ALGORITHM ParseQuery(input)

    IF input is empty
        THROW "Query is empty"

    Find first non-whitespace character
    Find last non-whitespace character

    IF input contains only whitespace
        THROW "Query is empty"

    Remove leading and trailing whitespace

    CREATE new Query

    IF query begins with GET
        SET query type to Filter
        SET query filter to ParseFilterQuery(input)

    ELSE
        SET query type to DotPath
        SET query dotPath to ParseDotPath(input)

    RETURN Query
```

FILTER query Parser:


```text
ALGORITHM ParseFilterQuery(input)

    SET position to beginning of input
    CREATE empty FilterQuery

    Skip whitespace

    IF GET keyword is not found
        THROW error

    Move past GET
    Skip whitespace

    Parse path after GET
    STORE as the selectField

    IF selectField is empty
        THROW error

    Skip whitespace

    IF FROM keyword is not found
        THROW error

    Move past FROM
    Skip whitespace

    Parse path after FROM
    STORE as a sourcePath

    IF sourcePath is empty
        THROW error

    Skip whitespace

    IF WHERE keyword exists

        Move past WHERE

        REPEAT

            CREATE new Condition
            Skip whitespace

            IF NOT keyword exists
                SET condition.negative to true
                Move past NOT
                Skip whitespace

            Parse condition field path

            IF field is empty
                THROW error

            Skip whitespace

            Read comparison operator
            Convert operator text to FilterOperator

            Skip whitespace

            IF no value exists
                THROW error

            IF value begins with ' or "
                Remember opening quote
                Read until matching closing quote

                IF closing quote is missing
                    THROW error

            ELSE
                Read value until whitespace

            STORE value in Condition
            ADD Condition to FilterQuery

            Skip whitespace

            IF AND keyword exists
                Move past AND
                CONTINUE parsing another condition

            ELSE IF OR keyword exists
                Mark previous condition's word operator as OR
                Move past OR
                CONTINUE parsing another condition

            ELSE
                STOP condition loop

    Skip remaining whitespace

    IF unread characters remain
        THROW error

    RETURN FilterQuery
```

parsing path pseudocode:

```text
ALGORITHM ParsePath(input, position)

    CREATE empty list of PathParts

    WHILE position has not reached end of input

        IF current character is whitespace
            STOP

        IF current character cannot begin a key
            THROW error

        Read key name
        ADD Key to path

        IF next character is [

            Move past [

            IF end of input reached
                THROW unclosed bracket error

            IF next character is *
                Move past *
                
                REQUIRE closing ]
                ADD AllElements to path

            ELSE IF next value is an array index
                Read index digits

                IF index is negative
                    THROW error

                REQUIRE closing ]

                ADD ArrayIndex to path

            ELSE
                THROW invalid array index error

        IF whitespace is reached
            STOP

        IF more input remains

            REQUIRE .

            Move past .

            IF input ends after .
                THROW trailing dot error

    RETURN path
```

### JSON PARSER pseudocode:

loading file:
```text
ALGORITHM LoadFile(filename)

    Open file in binary mode

    IF file cannot be opened
        Print error
        RETURN false

    Move to end of file
    Determine file size
    Move back to beginning

    Resize JSON buffer to file size
    Read entire file into buffer

    IF read fails
        Print error
        RETURN false

    RETURN true


```

finding which quotes are preceded by an odd number of backslashes:

```text
ALGORITHM FindOddBackSlash(backslashMask)

    Create masks for even and odd bit positions

    Identify the start of each run of backslashes

    Determine which runs begin on even positions
    Determine which runs begin on odd positions

    Compute the positions where an odd-length
    backslash run ends

    Combine those positions into one mask

    RETURN oddBackslashMask
```



```text
ALGORITHM FindString(quoteMask)

    Repeatedly XOR the quote mask with
    left-shifted versions of itself

    Propagate quote state across all 32 bit positions

    Resulting mask represents characters that
    are located inside JSON strings

    RETURN stringMask

```

Optimized SIMD scan of 32 bytes at a time.

```text
ALGORITHM IndexStructure()

    Prepare SIMD constants and lookup tables
    for JSON structural characters

    FOR each 32-byte block of JSON data

        Load 32 bytes into a SIMD register

        Split each byte into:
            low nibble
            high nibble

        Use lookup tables to identify
        potential JSON structural characters

        Combine lookup results into
        structural-character mask

        Compare all 32 bytes in parallel against:
            quote character
            backslash character

        Convert SIMD comparison results
        into 32-bit masks

        Find escaped quotes using FindOddBackSlash()

        Remove escaped quotes from quote mask

        Find regions that are inside strings
        using FindString()

        Remove structural characters that occur
        inside strings from the structural mask

        WHILE structural mask still contains positions

            Find position of next set bit

            Determine the actual character
            at that position

            IF character is {
                Add ObjectStart token

            ELSE IF character is }
                Add ObjectEnd token

            ELSE IF character is [
                Add ArrayStart token

            ELSE IF character is ]
                Add ArrayEnd token

            ELSE IF character is :
                Add Colon token

            ELSE IF character is ,
                Add Comma token

            Remove processed bit from mask

    END FOR
```

```text
ALGORITHM ConstructTree()

    FOR each token in typeIndex

        IF token is ObjectStart

            Create object node

            IF node stack is empty
                Set object as root
                Push root onto node stack

            ELSE IF current parent is an object
                Add object under current key
                Push new object onto stack

            ELSE IF current parent is an array
                Append object to array
                Push new object onto stack


        ELSE IF token is ArrayStart

            Create array node

            IF node stack is empty
                Set array as root
                Push root onto stack

            ELSE IF current parent is an object
                Add array under current key
                Push new array onto stack

            ELSE IF current parent is an array
                Append array
                Push new array onto stack


        ELSE IF token is String

            IF next token is Colon
                Treat current string as an object key
                Save key text

            ELSE
                Create string node
                Store start and end positions

                IF parent is an object
                    Attach string using current key

                ELSE IF parent is an array
                    Append string to array


        ELSE IF token is Number

            Create number node
            Store source positions

            Attach to current object or array


        ELSE IF token is Boolean

            Create boolean node
            Store source positions

            Attach to current object or array


        ELSE IF token is Null

            Create null node
            Store source positions

            Attach to current object or array


        ELSE IF token is ObjectEnd or ArrayEnd

            IF node stack is not empty
                Pop current container from stack

    END FOR
```

Printing:

```text
ALGORITHM PrintNode(node, depth)

    Determine indentation based on depth

    IF node is Object
        Print {
        FOR each child
            Print key
            Recursively PrintNode(child)
        Print }

    ELSE IF node is Array
        Print [
        FOR each child
            Recursively PrintNode(child)
        Print ]

    ELSE IF node is String
        Read original string from JSON buffer
        Print string

    ELSE IF node is Number
        Read original number from JSON buffer
        Print number

    ELSE IF node is Boolean
        Read original boolean from JSON buffer
        Print boolean

    ELSE IF node is Null
        Print null
```

recurive printing for debugging:

```text
ALGORITHM PrintTree()

    PrintNode(root, 0)
```
