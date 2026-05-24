find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

add_library(sl_gui STATIC
    # Source files
    OcrWorker.cpp
    DropZone.cpp
    SearchWidget.cpp
    ArchiveWidget.cpp
    MainWindow.cpp

    # Headers listed explicitly so AUTOMOC finds Q_OBJECT classes
    ${PROJECT_SOURCE_DIR}/include/gui/OcrWorker.h
    ${PROJECT_SOURCE_DIR}/include/gui/DropZone.h
    ${PROJECT_SOURCE_DIR}/include/gui/SearchWidget.h
    ${PROJECT_SOURCE_DIR}/include/gui/ArchiveWidget.h
    ${PROJECT_SOURCE_DIR}/include/gui/MainWindow.h
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
