# Qt6 is found at the root CMakeLists level
# Components are already found — just reference them here

add_library(sl_gui STATIC
    ${PROJECT_SOURCE_DIR}/include/gui/OcrWorker.h OcrWorker.cpp
    ${PROJECT_SOURCE_DIR}/include/gui/DropZone.h DropZone.cpp
    ${PROJECT_SOURCE_DIR}/include/gui/SearchWidget.h SearchWidget.cpp
    ${PROJECT_SOURCE_DIR}/include/gui/ArchiveWidget.h ArchiveWidget.cpp
    ${PROJECT_SOURCE_DIR}/include/gui/MainWindow.h MainWindow.cpp
)

target_include_directories(sl_gui PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sl_gui
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        sl_ocr
        sl_search
        sl_pdf
)

target_compile_features(sl_gui PUBLIC cxx_std_17)
