## Pseudocode:


### Query Parser Pseudocode:

Main parser:

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
