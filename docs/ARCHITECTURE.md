# SmartLibrarian — Architecture Notes

These are the design decisions I made and why. Useful context
if you're reading the source code.

---

## Module Dependency Graph

```text
main.cpp
│
├── gui/MainWindow
│       ├── gui/OcrWorker ──────► ocr/OcrEngine
│       │                              └── ocr/ImagePreprocessor
│       ├── gui/SearchWidget ───► search/SearchEngine
│       │                              ├── search/Trie
│       │                              ├── search/InvertedIndex
│       │                              └── search/TextProcessor
│       ├── gui/ArchiveWidget
│       └── gui/DropZone
│
├── archive/PersistenceManager
│       ├── archive/BinaryWriter
│       └── archive/BinaryReader
│
└── pdf/PdfDocument
├── pdf/PdfWriter
├── pdf/PdfPage
└── pdf/PdfStream
```

Dependencies flow downward only. No circular dependencies.

---

## Key Design Decisions

### Why custom PDF writer instead of libharu/PDFium?

Using a library would hide the interesting part. The PDF spec is
publicly available. Writing the serializer from scratch meant
understanding cross-reference tables, object numbering, stream
length calculation, and binary file format design — all of which
are transferable concepts.

### Why Trie + InvertedIndex instead of just SQLite FTS?

Same reason. SQLite FTS is a correct, production-ready solution.
A custom Trie + InvertedIndex is an opportunity to implement
data structures from scratch and understand their complexity
characteristics at the implementation level, not just the theory level.

### Why custom binary persistence instead of JSON?

JSON is 2-3x larger on disk and 3-5x slower to parse for our use case
(many integers, repeated short strings). The custom format also
demonstrates format versioning and magic-number validation —
concepts that appear in every production binary format.

### Why Qt6 for the GUI?

Qt's signal/slot system with automatic cross-thread queuing is
genuinely elegant for this use case. The alternative (raw pthreads +
callbacks + locks) would have been more code with more opportunity
for bugs. Qt's worker-object pattern (`moveToThread`) maps cleanly
to the problem.

### Memory ownership conventions

- `unique_ptr`: exclusive ownership (PdfDocument owns pages, 
  MainWindow owns OcrEngine)
- Raw pointer with comment "borrowed": non-owning reference 
  that the caller guarantees outlives the borrower
  (SearchWidget holds raw ptr to SearchEngine owned by MainWindow)
- Qt parent/child tree: QObject-derived widgets owned by their 
  Qt parent — never wrap in unique_ptr if they have a Qt parent

---

## Threading Model

```text
Main Thread (Qt event loop)
All widget reads and writes happen here.
Never block this thread.
OCR Thread (QThread)
OcrWorker::process() runs here.
Never touch widgets from this thread.
Communicate back via signals only.
```

Cross-thread signal/slot connections are automatically queued by Qt.
No mutexes needed for the communication itself. The OcrEngine is
accessed only from the OCR thread after initial construction.

---

## Binary Format Spec

### index.slidx

```text
Offset  Size  Type     Description
0       8     bytes    Magic: "SLIDX001"
8       4     uint32   Format version (currently 1)
12      4     uint32   Document count N
16      ...   records  N document records:
4   uint32  Document ID
var string  Path (length-prefixed UTF-8)
var string  Title
4   uint32  Total word count
...     4     uint32   Unique word count M
...     ...   records  M word records:
var string  Word
4   uint32  Posting count P
...         P posting records:
4  uint32  Document ID
4  uint32  Term frequency
4  uint32  First position
```

String encoding: `[uint32_t length][char* bytes]`
No null terminator. All integers little-endian.

### archive.slarc

```text
Offset  Size  Type     Description
0       8     bytes    Magic: "SLARC001"
8       4     uint32   Format version
12      4     uint32   Document count N
16      ...   records  N document records:
4   uint32  ID
var string  Filename
var string  Full path
var string  Extracted text
4   float32 OCR confidence (IEEE 754)
4   uint32  Word count
var string  Timestamp
```

---

## Test Strategy

Each module is tested in isolation. Tests don't share state between
test cases — GoogleTest fixtures provide fresh setup for each test.

The most important tests are the round-trip tests in
`tests/unit/archive/test_BinaryWriterReader.cpp` — they verify
that every value survives a write/read cycle exactly, including
edge cases like 0, max values, and empty strings.

The search engine ranking tests are next in importance — they
assert not just that results are non-empty but that the most
relevant document actually ranks first.
