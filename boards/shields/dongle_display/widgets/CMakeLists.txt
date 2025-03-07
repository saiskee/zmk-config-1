target_sources(app PRIVATE
    split_bongo_cat.c
    output_status.c
    output_status_sym.c
    modifiers.c
    modifiers_sym.c
    hid_indicators.c
    layer_status.c
    bongo_cat_images.c
    battery_status.c
    bongo_cat.c
    key_info_widget.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
