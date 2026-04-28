idf_component_register(SRCS "main.cpp" 
                            "Core/Protection.cpp" 
                            "Core/StateManager.cpp"
                            "HAL/UartLink.cpp"
                            "HAL/Display.cpp"
                            "Services/NetManager.cpp"
                       INCLUDE_DIRS "." "include" "Core" "HAL" "Services"
                       REQUIRES nvs_flash esp_wifi mqtt esp_driver_usb_serial_jtag u8g2 driver)