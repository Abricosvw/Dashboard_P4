# Добавляем background_task.c в список исходных файлов для компиляции
SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    screens/ui_Screen3.c
    screens/ui_Screen4.c
    screens/ui_Screen5.c
    screens/ui_Screen6.c
    ui.c
    settings_config.c
    ui_screen_manager.c
    components/ui_comp_hook.c
    ui_helpers.c
    ../background_task.c 
)

# Компилируем все исходники в библиотеку ui
add_library(ui ${SOURCES})

# Указываем, где лежат заголовочные файлы (включая background_task.h)
target_include_directories(ui PUBLIC 
    ../
    ./
    ./screens
    ./components
)