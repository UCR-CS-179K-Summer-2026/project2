// Standalone benchmark harness -- not part of the GoogleTest suite.
// Compile and run TWICE against the same file: once as the normal
// (optimized) build, once with -DBENCH_NAIVE, then compare the two
// printed/CSV results. Same tree, same file, same queries -- the only
// thing that differs between the two binaries is whether evaluateCondition
// or evaluateConditionNaive runs inside WHERE evaluation.
//
// Usage: ./benchmark_runner <path-to-json-file> [csv-output-path]

#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryParser.h"
#include "query_executor.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

struct QueryBench {
    std::string label;
    std::string queryStr;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-json-file> [csv-output-path]\n";
        return 1;
    }
    std::string filePath = argv[1];
    std::string csvPath = (argc >= 3) ? argv[2] : "";

#ifdef BENCH_NAIVE
    const std::string buildLabel = "naive (pre-Sprint-2)";
#elif defined(BENCH_PRE_OPT)
    // Provisional label -- rename this string (only this string, no code
    // change needed) once this round of optimizations is considered
    // final, e.g. to "pre-Sprint-4".
    const std::string buildLabel = "pre-optimization (in progress)";
#else
    const std::string buildLabel = "post-optimization (current)";
#endif

    std::cout << "== Benchmark: " << filePath << " [" << buildLabel << "] ==\n";

    // ---- Parse phase timing (3 iterations, averaged) ----
    const int PARSE_ITERATIONS = 3;
    double totalLoadMs = 0, totalIndexMs = 0, totalTreeMs = 0;
    size_t fileSizeBytes = 0;

    parser p;  // keep the LAST run's parser around for the query phase below
    for (int iter = 0; iter < PARSE_ITERATIONS; ++iter) {
        parser localP;

        auto t0 = Clock::now();
        if (!localP.loadFile(filePath)) {
            std::cerr << "Failed to load file: " << filePath << "\n";
            return 1;
        }
        totalLoadMs += msSince(t0);

        auto t1 = Clock::now();
        localP.indexStructure();
        totalIndexMs += msSince(t1);

        auto t2 = Clock::now();
        localP.constructTree();
        totalTreeMs += msSince(t2);

        fileSizeBytes = localP.getJsonData().size();
        if (iter == PARSE_ITERATIONS - 1) p = std::move(localP);
    }

    double avgLoadMs = totalLoadMs / PARSE_ITERATIONS;
    double avgIndexMs = totalIndexMs / PARSE_ITERATIONS;
    double avgTreeMs = totalTreeMs / PARSE_ITERATIONS;
    double avgParseMs = avgLoadMs + avgIndexMs + avgTreeMs;
    double fileSizeMb = fileSizeBytes / (1024.0 * 1024.0);

    std::cout << "\n-- Parse phase (avg of " << PARSE_ITERATIONS << " runs, "
              << fileSizeMb << " MB) --\n";
    std::cout << "  loadFile:        " << avgLoadMs << " ms\n";
    std::cout << "  indexStructure:  " << avgIndexMs << " ms\n";
    std::cout << "  constructTree:   " << avgTreeMs << " ms\n";
    std::cout << "  TOTAL parse:     " << avgParseMs << " ms\n";

    // ---- Query phase timing ----
    // Schema assumes store.products[] with name/price/inStock/category,
    // matching generate_benchmark_data.py's output.
    std::vector<QueryBench> queries = {
        {"dot-path point lookup",  "store.name"},
        {"wildcard fan-out",       "store.products[*].name"},
        {"filter: numeric WHERE",  "GET name FROM store.products WHERE price > 1000"},
        {"filter: AND",            "GET name FROM store.products WHERE category = electronics AND inStock = true"},
        {"filter: OR",             "GET name FROM store.products WHERE category = furniture OR price > 1800"},
        // [NEW] long-string WHERE comparison (~75-char values, well past
        // small-string-optimization threshold), added to isolate whether
        // the Sprint 2 zero-copy optimization shows a real gap once field
        // values are long enough to force heap allocation on the naive
        // path -- short fields (category, inStock) showed none.
        {"filter: long-string WHERE", "GET name FROM store.products WHERE description = nomatch"},
    };

    const int QUERY_ITERATIONS = 200;
    QueryParser qp;
    std::vector<std::pair<std::string, double>> queryResults;

    std::cout << "\n-- Query phase (avg of " << QUERY_ITERATIONS << " runs each) --\n";
    for (const auto& qb : queries) {
        Query parsedQuery = qp.parse(qb.queryStr);
        double total = 0;
        size_t resultCount = 0;
        for (int i = 0; i < QUERY_ITERATIONS; ++i) {
            auto t0 = Clock::now();
            auto results = executeQuery(p.getRoot(), parsedQuery, p.getJsonData());
            total += msSince(t0);
            resultCount = results.size();
        }
        double avgMs = total / QUERY_ITERATIONS;
        queryResults.push_back({qb.label, avgMs});
        std::cout << "  " << qb.label << ": " << avgMs << " ms  ("
                  << resultCount << " results)\n";
    }

    // ---- CSV output for the website table/chart ----
    if (!csvPath.empty()) {
        bool fileExists = std::ifstream(csvPath).good();
        std::ofstream csv(csvPath, std::ios::app);
        if (!fileExists) {
            csv << "file,size_mb,build,phase,time_ms\n";
        }
        csv << filePath << "," << fileSizeMb << "," << buildLabel << ",loadFile," << avgLoadMs << "\n";
        csv << filePath << "," << fileSizeMb << "," << buildLabel << ",indexStructure," << avgIndexMs << "\n";
        csv << filePath << "," << fileSizeMb << "," << buildLabel << ",constructTree," << avgTreeMs << "\n";
        csv << filePath << "," << fileSizeMb << "," << buildLabel << ",total_parse," << avgParseMs << "\n";
        for (const auto& [label, ms] : queryResults) {
            csv << filePath << "," << fileSizeMb << "," << buildLabel << "," << label << "," << ms << "\n";
        }
        std::cout << "\nAppended results to " << csvPath << "\n";
    }

    return 0;
}