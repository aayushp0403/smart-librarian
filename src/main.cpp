/**
 * SmartLibrarian — Phase 1 Entry Point
 *
 * Demonstrates the custom PDF engine:
 * Generates a real, multi-page PDF from scratch.
 */

#include <iostream>
#include "core/Application.h"
#include "pdf/PdfDocument.h"
#include "utils/Logger.h"

int main(int argc, char* argv[])
{
    using namespace sl;

    utils::Logger::info("SmartLibrarian starting — Phase 1: PDF Engine");

    // ─────────────────────────────────────────────
    // Demonstrate the PDF engine
    // ─────────────────────────────────────────────

    pdf::PdfDocument doc;

    // Register fonts — must be done before adding pages
    doc.addFont("F1", "Helvetica");
    doc.addFont("F2", "Helvetica-Bold");
    doc.addFont("F3", "Courier");

    // ── PAGE 1 ──────────────────────────────────
    {
        pdf::PdfPage& page = doc.addPage();

        // Draw a header bar
        page.setFillColor(0.2f, 0.4f, 0.8f);   // Blue
        page.fillRect(0, 750, 612, 42);

        // Title text in white on the blue bar
        page.beginText();
        page.setTextColor(1.0f, 1.0f, 1.0f);   // White
        page.setFont("F2", 22.0f);
        page.moveTo(72.0f, 762.0f);
        page.showText("Smart Librarian — Document Archive");
        page.endText();

        // Separator line
        page.setStrokeColor(0.2f, 0.4f, 0.8f);
        page.drawLine(72, 730, 540, 730, 1.5f);

        // Body text
        page.beginText();
        page.setTextColor(0.1f, 0.1f, 0.1f);   // Near-black
        page.setFont("F2", 14.0f);
        page.moveTo(72.0f, 700.0f);
        page.showText("Phase 1: Custom PDF Engine");

        page.setFont("F1", 11.0f);
        page.setLeading(18.0f);
        page.moveTo(72.0f, 675.0f);

        const char* lines[] = {
            "This PDF was generated entirely in C++ without any PDF library.",
            "Every byte was written manually using our custom serialization engine.",
            "",
            "Technical implementation:",
            "  - PdfStream: binary file I/O with exact byte position tracking",
            "  - PdfPage: content stream builder using PostScript-derived operators",
            "  - PdfWriter: object model serializer with cross-reference table",
            "  - PdfDocument: high-level RAII document API",
            "",
            "The cross-reference table at the end of this file records the exact",
            "byte offset of every PDF object, enabling O(1) random access by readers.",
            nullptr
        };

        for (int i = 0; lines[i] != nullptr; ++i) {
            page.showText(lines[i]);
            page.nextLine();
        }
        page.endText();

        // Footer
        page.setStrokeColor(0.7f, 0.7f, 0.7f);
        page.drawLine(72, 60, 540, 60, 0.5f);
        page.beginText();
        page.setTextColor(0.5f, 0.5f, 0.5f);
        page.setFont("F1", 9.0f);
        page.moveTo(72.0f, 45.0f);
        page.showText("SmartLibrarian v0.1.0 — Page 1");
        page.endText();
    }

    // ── PAGE 2 ──────────────────────────────────
    {
        pdf::PdfPage& page = doc.addPage();

        page.setFillColor(0.2f, 0.4f, 0.8f);
        page.fillRect(0, 750, 612, 42);

        page.beginText();
        page.setTextColor(1.0f, 1.0f, 1.0f);
        page.setFont("F2", 22.0f);
        page.moveTo(72.0f, 762.0f);
        page.showText("PDF Object Model — Technical Reference");
        page.endText();

        page.setStrokeColor(0.2f, 0.4f, 0.8f);
        page.drawLine(72, 730, 540, 730, 1.5f);

        page.beginText();
        page.setTextColor(0.1f, 0.1f, 0.1f);
        page.setFont("F2", 12.0f);
        page.moveTo(72.0f, 705.0f);
        page.showText("PDF Object Types:");

        page.setFont("F3", 10.0f);   // Courier for code
        page.setLeading(16.0f);
        page.moveTo(72.0f, 685.0f);

        const char* techLines[] = {
            "  Boolean    true / false",
            "  Integer    42",
            "  Real       3.14159",
            "  String     (Hello, World!)",
            "  Name       /Type",
            "  Array      [0 0 612 792]",
            "  Dictionary << /Type /Page /Parent 2 0 R >>",
            "  Stream     << /Length 44 >> stream ... endstream",
            nullptr
        };

        for (int i = 0; techLines[i] != nullptr; ++i) {
            page.showText(techLines[i]);
            page.nextLine();
        }

        page.setFont("F2", 12.0f);
        page.moveTo(72.0f, 530.0f);
        page.showText("Content Stream Operators Used:");

        page.setFont("F3", 10.0f);
        page.setLeading(16.0f);
        page.moveTo(72.0f, 510.0f);

        const char* opLines[] = {
            "  BT / ET       Begin/End text block",
            "  /Fn sz Tf     Set font and size",
            "  x y Td        Move text cursor",
            "  sz TL         Set line leading",
            "  T*            Move to next line",
            "  (str) Tj      Show string",
            "  r g b rg      Set fill color (RGB)",
            "  r g b RG      Set stroke color (RGB)",
            "  w W           Set line width",
            "  x y m         Moveto (path)",
            "  x y l         Lineto (path)",
            "  S             Stroke path",
            "  x y w h re    Append rectangle",
            "  f             Fill path",
            nullptr
        };

        for (int i = 0; opLines[i] != nullptr; ++i) {
            page.showText(opLines[i]);
            page.nextLine();
        }
        page.endText();

        page.setStrokeColor(0.7f, 0.7f, 0.7f);
        page.drawLine(72, 60, 540, 60, 0.5f);
        page.beginText();
        page.setTextColor(0.5f, 0.5f, 0.5f);
        page.setFont("F1", 9.0f);
        page.moveTo(72.0f, 45.0f);
        page.showText("SmartLibrarian v0.1.0 — Page 2");
        page.endText();
    }

    // ── Save ────────────────────────────────────
    const std::string outputPath = "SmartLibrarian_Phase1.pdf";
    doc.save(outputPath);

    utils::Logger::info("PDF generated: " + outputPath);
    utils::Logger::info("Pages: " + std::to_string(doc.pageCount()));
    utils::Logger::info("Open the PDF to verify the output.");

    // ─────────────────────────────────────────────
    // Application lifecycle (existing infrastructure)
    // ─────────────────────────────────────────────
    core::Application app(argc, argv);
    return app.run();
}
