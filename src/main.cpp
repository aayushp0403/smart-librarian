/**
 * SmartLibrarian — Phase 2 Entry Point
 *
 * Demonstrates:
 *   Phase 1: Custom PDF engine
 *   Phase 2: Search engine (Trie + Inverted Index + TF-IDF)
 */

#include <iostream>
#include <iomanip>
#include "core/Application.h"
#include "pdf/PdfDocument.h"
#include "search/SearchEngine.h"
#include "utils/Logger.h"

// ─────────────────────────────────────────────────────────────────
// Sample documents — simulating OCR output from scanned pages
// ─────────────────────────────────────────────────────────────────

static const struct { const char* path; const char* title; const char* content; }
k_sampleDocs[] = {
    {
        "docs/neural_networks.txt",
        "Introduction to Neural Networks",
        "Neural networks are computing systems inspired by biological neural networks "
        "that constitute animal brains. A neural network consists of layers of "
        "interconnected nodes or neurons. Deep learning uses neural networks with "
        "many layers. Backpropagation trains neural networks by computing gradients. "
        "Convolutional neural networks excel at image recognition tasks. "
        "Recurrent neural networks process sequential data like text and speech."
    },
    {
        "docs/algorithms.txt",
        "Algorithm Design and Complexity",
        "An algorithm is a finite sequence of instructions for solving a problem. "
        "Algorithm complexity is measured using Big-O notation. Binary search runs "
        "in O(log n) time on sorted arrays. Quicksort has average O(n log n) "
        "complexity but worst-case O(n squared). Dynamic programming solves "
        "problems by breaking them into overlapping subproblems. Graph algorithms "
        "like Dijkstra find shortest paths in weighted graphs."
    },
    {
        "docs/memory_management.txt",
        "C++ Memory Management",
        "Memory management in C++ involves stack and heap allocation. Stack "
        "allocation is automatic and fast. Heap allocation uses new and delete "
        "operators. Smart pointers manage heap memory automatically. unique_ptr "
        "provides exclusive ownership semantics. shared_ptr uses reference counting "
        "for shared ownership. RAII ensures resources are released when objects "
        "go out of scope. Memory leaks occur when heap memory is never freed."
    },
    {
        "docs/data_structures.txt",
        "Fundamental Data Structures",
        "Data structures organize data for efficient access and modification. "
        "Arrays provide O(1) random access with contiguous memory layout. "
        "Linked lists allow O(1) insertion but O(n) access. Hash tables provide "
        "O(1) average lookup using hash functions. Binary search trees maintain "
        "sorted order with O(log n) operations. Heaps implement priority queues "
        "efficiently. Graphs represent relationships between entities. "
        "A trie is a tree structure optimized for string prefix operations."
    },
    {
        "docs/pdf_internals.txt",
        "PDF File Format Internals",
        "PDF is a binary file format for document exchange. A PDF file contains "
        "objects: booleans, integers, strings, names, arrays, dictionaries, "
        "and streams. The cross-reference table maps object numbers to byte "
        "offsets for random access. Content streams contain PostScript-like "
        "drawing operators. Text rendering uses font resources and coordinate "
        "transformations. PDF readers parse the trailer to find the catalog "
        "object which is the document root."
    }
};

static constexpr int k_docCount = 5;

// ─────────────────────────────────────────────────────────────────
// Helper: print search results
// ─────────────────────────────────────────────────────────────────

static void printResults(
    const std::vector<sl::search::SearchResult>& results,
    std::string_view query
)
{
    std::cout << "\n┌─────────────────────────────────────────────────────┐\n";
    std::cout << "│ Query: \"" << query << "\"\n";
    std::cout << "│ Results: " << results.size() << "\n";
    std::cout << "├─────────────────────────────────────────────────────┤\n";

    if (results.empty()) {
        std::cout << "│  No results found.\n";
    }

    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        std::cout << "│ " << std::setw(2) << (i + 1) << ". "
                  << r.title << "\n";
        std::cout << "│     Score: " << std::fixed << std::setprecision(4)
                  << r.score
                  << "  Matched terms: " << r.matchCount << "\n";
        std::cout << "│     Path: " << r.path << "\n";
    }

    std::cout << "└─────────────────────────────────────────────────────┘\n";
}

// ─────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    using namespace sl;

    utils::Logger::info("SmartLibrarian — Phase 2: Search Engine Demo");

    // ── PHASE 2: Search Engine ───────────────────────────────────

    search::SearchEngine engine;

    // Index all sample documents
    utils::Logger::info("Indexing documents...");
    for (int i = 0; i < k_docCount; ++i) {
        search::DocumentID id = engine.indexDocument(
            k_sampleDocs[i].path,
            k_sampleDocs[i].title,
            k_sampleDocs[i].content
        );
        std::cout << "  Indexed [ID=" << id << "] "
                  << k_sampleDocs[i].title << "\n";
    }

    std::cout << "\n  Total documents : " << engine.documentCount()  << "\n";
    std::cout << "  Unique words    : " << engine.uniqueWordCount() << "\n";
    std::cout << "  Trie nodes      : " << engine.trieNodeCount()   << "\n";

    // ── Run search queries ───────────────────────────────────────

    // Query 1: single high-relevance term
    printResults(engine.search("neural"), "neural");

    // Query 2: multi-term — should rank neural doc highest
    printResults(engine.search("neural network layers"), "neural network layers");

    // Query 3: data structures query
    printResults(engine.search("hash table binary search"), "hash table binary search");

    // Query 4: cross-document term (memory appears in multiple docs)
    printResults(engine.search("memory allocation"), "memory allocation");

    // Query 5: no results
    printResults(engine.search("quantum entanglement"), "quantum entanglement");

    // ── Prefix search (autocomplete) ─────────────────────────────
    std::cout << "\n── Prefix Search (autocomplete) ──────────────────────\n";

    auto showPrefix = [&](std::string_view prefix) {
        auto words = engine.prefixSearch(prefix, 8);
        std::cout << "  \"" << prefix << "\" → [";
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << words[i];
        }
        std::cout << "]\n";
    };

    showPrefix("ne");
    showPrefix("mem");
    showPrefix("alg");
    showPrefix("comp");
    showPrefix("tr");

    // ── PHASE 1: Generate PDF of search results ──────────────────
    utils::Logger::info("\nGenerating search results PDF...");

    pdf::PdfDocument doc;
    doc.addFont("F1", "Helvetica");
    doc.addFont("F2", "Helvetica-Bold");
    doc.addFont("F3", "Courier");

    {
        pdf::PdfPage& page = doc.addPage();

        // Header bar
        page.setFillColor(0.15f, 0.35f, 0.65f);
        page.fillRect(0, 750, 612, 42);

        page.beginText();
        page.setTextColor(1.0f, 1.0f, 1.0f);
        page.setFont("F2", 20.0f);
        page.moveTo(72.0f, 762.0f);
        page.showText("SmartLibrarian — Search Index Report");
        page.endText();

        page.setStrokeColor(0.15f, 0.35f, 0.65f);
        page.drawLine(72, 730, 540, 730, 1.0f);

        page.beginText();
        page.setTextColor(0.1f, 0.1f, 0.1f);
        page.setFont("F2", 12.0f);
        page.moveTo(72.0f, 705.0f);
        page.showText("Index Statistics");

        page.setFont("F1", 11.0f);
        page.setLeading(18.0f);
        page.moveTo(72.0f, 685.0f);
        page.showText("Documents indexed : " +
                      std::to_string(engine.documentCount()));
        page.nextLine();
        page.showText("Unique words      : " +
                      std::to_string(engine.uniqueWordCount()));
        page.nextLine();
        page.showText("Trie nodes        : " +
                      std::to_string(engine.trieNodeCount()));
        page.nextLine();
        page.nextLine();

        page.setFont("F2", 12.0f);
        page.showText("Query: \"neural network layers\"");
        page.nextLine();

        auto results = engine.search("neural network layers", 5);
        page.setFont("F3", 10.0f);
        page.setLeading(16.0f);
        for (std::size_t i = 0; i < results.size(); ++i) {
            std::string line =
                std::to_string(i + 1) + ". " +
                results[i].title +
                "  [score: " +
                std::to_string(results[i].score).substr(0, 6) + "]";
            page.showText(line);
            page.nextLine();
        }
        page.endText();

        page.setStrokeColor(0.7f, 0.7f, 0.7f);
        page.drawLine(72, 60, 540, 60, 0.5f);
        page.beginText();
        page.setTextColor(0.5f, 0.5f, 0.5f);
        page.setFont("F1", 9.0f);
        page.moveTo(72.0f, 45.0f);
        page.showText("SmartLibrarian v0.1.0 — Phase 2 Report");
        page.endText();
    }

    doc.save("SmartLibrarian_Phase2.pdf");
    utils::Logger::info("PDF saved: SmartLibrarian_Phase2.pdf");

    core::Application app(argc, argv);
    return app.run();
}
