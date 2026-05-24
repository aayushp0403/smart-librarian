add_library(sl_pdf STATIC
    PdfStream.cpp
    PdfPage.cpp
    PdfWriter.cpp
    PdfDocument.cpp
)

target_include_directories(sl_pdf PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(sl_pdf PUBLIC cxx_std_17)