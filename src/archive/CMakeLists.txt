add_library(sl_archive STATIC
    BinaryWriter.cpp
    BinaryReader.cpp
    PersistenceManager.cpp

    ${PROJECT_SOURCE_DIR}/include/archive/BinaryWriter.h
    ${PROJECT_SOURCE_DIR}/include/archive/BinaryReader.h
    ${PROJECT_SOURCE_DIR}/include/archive/PersistenceManager.h
    ${PROJECT_SOURCE_DIR}/include/archive/AppConfig.h
)

target_include_directories(sl_archive PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sl_archive
    PUBLIC
        sl_search
        sl_utils
        sl_gui
)

target_compile_features(sl_archive PUBLIC cxx_std_17)
