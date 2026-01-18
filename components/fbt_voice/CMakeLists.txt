set(MAIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../main")
get_filename_component(MAIN_DIR ${MAIN_DIR} ABSOLUTE)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(IMAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/images")
set(PRIVATE_INCLUDE_DIRS
    "${MAIN_DIR}"
    "${MAIN_DIR}/assets"
    "${MAIN_DIR}/protocols"
    "${MAIN_DIR}/boards/common"
    "${MAIN_DIR}/display"
    "${MAIN_DIR}/audio"
    "${MAIN_DIR}/audio/codecs"
    "${MAIN_DIR}/audio/processors"
    "${MAIN_DIR}/audio/wake_words"
    "${MAIN_DIR}/display/lvgl_display" 
    "${MAIN_DIR}/display/lvgl_display/jpg"  
)



# 获取图片资源
file(GLOB_RECURSE COMMON_IMAGES 
    "${IMAGES_DIR}/*.jpg" 
    "${IMAGES_DIR}/*.png"
    "${IMAGES_DIR}/*.jpeg"
)

# 获取所有 FBT 源文件（更精确的方式）
file(GLOB_RECURSE FBT_C_FILES 
    "${SRC_DIR}/*.c"
    "${SRC_DIR}/*/*.c"
    "${SRC_DIR}/*/*/*.c"
)

file(GLOB_RECURSE FBT_CPP_FILES 
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/*/*.cpp"
    "${SRC_DIR}/*/*/*.cpp"
    "${SRC_DIR}/*.cc"
    "${SRC_DIR}/*/*.cc"
    "${SRC_DIR}/*/*/*.cc"
)

list(APPEND SOURCES ${FBT_C_FILES} ${FBT_CPP_FILES})
list(REMOVE_DUPLICATES COMMON_IMAGES)



file(GLOB_RECURSE INCLUDE_SUBDIRS LIST_DIRECTORIES true "include/*")

set(ALL_INCLUDES "include")
foreach(dir ${INCLUDE_SUBDIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND ALL_INCLUDES ${dir})
    endif()
endforeach()

# 关键：注册组件
idf_component_register(
    SRCS ${SOURCES}                    
    EMBED_FILES ${COMMON_IMAGES}    
    
    INCLUDE_DIRS  "${ALL_INCLUDES}" 
    
    REQUIRES
        freertos
        nvs_flash
        esp_lcd
        lvgl
        esp_lcd_touch_cst816s
        json
        esp-ml307
        espressif__esp-sr
        78__esp-opus-encoder
        78__esp-opus
        78__esp-wifi-connect
        
    PRIV_REQUIRES
        driver
)

target_include_directories(${COMPONENT_LIB} PRIVATE ${PRIVATE_INCLUDE_DIRS})

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LV_LVGL_H_INCLUDE_SIMPLE
)