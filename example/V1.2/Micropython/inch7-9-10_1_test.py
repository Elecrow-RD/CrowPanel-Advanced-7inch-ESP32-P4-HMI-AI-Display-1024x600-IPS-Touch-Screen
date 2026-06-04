import machine
from machine import Pin
from i2c import I2C
import time
import sys
import lvgl as lv
import lcd
import display_driver_framework

# Import your peripheral driver modules
from my_gt911 import GT911
from dht20 import DHT20
import ui_images


# --------------------------
# Hardware Parameter Configuration
# --------------------------
WIDTH = 1024
HEIGHT = 600
I2C_BUS = 0
LCD_BK_POWER = 29
I2C_SDA_PIN = 45
I2C_SCL_PIN = 46
I2C_FREQ = 400 * 1000
TOUCH_RST_PIN = 40
TOUCH_INT_PIN = 42

lcd_bk_power = machine.Pin(LCD_BK_POWER, machine.Pin.OUT, value=0)
lcd_bk_power(0)     # Enable screen power circuit
LED = Pin(48, Pin.OUT)  # Set GPIO pin 48 to output mode

# --------------------------
# Initialize Hardware Devices
# --------------------------
def device_init():
    lv.init()
    lcd.init()
    lcd.clear(0xFFFFFF) # Fill the entire screen with white
    lcd.backlight_set(100) # Enable backlight (0~100)
    print("✅ MIPI LCD hardware initialization completed")

    # Create LVGL display instance
    display = lv.display_create(WIDTH, HEIGHT)
    display.set_default()

    # Configure display buffers and flush callback
    BUFFER_SIZE = (WIDTH * HEIGHT * 3)
    draw_buf1 = bytearray(BUFFER_SIZE)
    draw_buf2 = bytearray(BUFFER_SIZE)

    display.set_buffers(
        draw_buf1, 
        draw_buf2, 
        len(draw_buf1), 
        lv.DISPLAY_RENDER_MODE.PARTIAL
    )

    def flush_cb(display, area, color_p):
        w = area.x2 - area.x1 + 1
        h = area.y2 - area.y1 + 1
        size_in_bytes = w * h * 3
        try:
            data_view = color_p.__dereference__(size_in_bytes)
            lcd.flush(area.x1, area.y1, area.x2, area.y2, data_view)
        except Exception as e:
            print(f"❌ LCD refresh exception: {e}")
        display.flush_ready()

    display.set_flush_cb(flush_cb)
    print("✅ LVGL display initialization completed")

    i2c_bus = I2C.Bus(
        host=I2C_BUS,
        scl=I2C_SCL_PIN,
        sda=I2C_SDA_PIN,
        freq=I2C_FREQ,
    )
    print(f"\n📡 Devices on I2C bus: {[hex(d) for d in i2c_bus.scan()]}")

    i2c_dht20 = I2C.Device(
        bus = i2c_bus, 
        dev_id = DHT20.I2C_ADDR, 
        reg_bits=8,
    )

    # Initialize DHT20 sensor
    try :
        dht20 = DHT20(i2c_dht20)
    except Exception as e:
        sys.print_exception(e)
        print('Failed to initialize DHT20 sensor!')

    # Initialize GT911 touch driver
    device_gt911 = I2C.Device(
        bus=i2c_bus,
        dev_id=GT911.I2C_ADDR,
        reg_bits=16,
    )

    touch = GT911(
        device=device_gt911,
        reset_pin=TOUCH_RST_PIN,
        interrupt_pin=TOUCH_INT_PIN,
    )

    indev = lv.indev_create()
    indev.set_type(lv.INDEV_TYPE.POINTER)
    indev.set_read_cb(touch._get_coords)
    # indev.set_display(display)  # NOQA
    # indev.enable(False)  # NOQA

    return display, touch, dht20


# --------------------------
# LVGL UI Functions
# --------------------------
def SetFlag(obj, flag, value):
    if (value):
        obj.add_flag(flag)
    else:
        obj.remove_flag(flag)
    return

def Button1_eventhandler(event_struct):
   event = event_struct.get_code()
   if event == lv.EVENT.CLICKED and True:
      LED.value(1)
   return

def Button2_eventhandler(event_struct):
   event = event_struct.get_code()
   if event == lv.EVENT.CLICKED and True:
      LED.value(0)
   return

def create_lvgl_ui():
    ui_Screen1 = lv.obj()
    SetFlag(ui_Screen1, lv.obj.FLAG.SCROLLABLE, False)
    ui_Screen1.set_style_bg_color(lv.color_hex(0xF0E5DA), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Screen1.set_style_bg_opa(255, lv.PART.MAIN | lv.STATE.DEFAULT)

    ui_Button1 = lv.image(ui_Screen1)
    ui_Button1.set_src(ui_images.ui_img_on_png)
    ui_Button1.set_width(85)
    ui_Button1.set_height(85)
    ui_Button1.set_x(208)
    ui_Button1.set_y(-70)
    ui_Button1.set_align(lv.ALIGN.CENTER)
    SetFlag(ui_Button1, lv.obj.FLAG.SCROLLABLE, False)
    SetFlag(ui_Button1, lv.obj.FLAG.SCROLL_ON_FOCUS, True)
    SetFlag(ui_Button1, lv.obj.FLAG.CLICKABLE, True)
    ui_Button1.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.set_style_bg_grad_color(lv.color_hex(0xFFE032), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.set_style_bg_color(lv.color_hex(0xFFE032), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.set_style_bg_opa(255, lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button1.add_event_cb(Button1_eventhandler, lv.EVENT.ALL, None)

    ui_Button2 = lv.image(ui_Screen1)
    ui_Button2.set_src(ui_images.ui_img_off_png)
    ui_Button2.set_width(85)
    ui_Button2.set_height(85)
    ui_Button2.set_x(205)
    ui_Button2.set_y(75)
    ui_Button2.set_align(lv.ALIGN.CENTER)
    SetFlag(ui_Button2, lv.obj.FLAG.SCROLLABLE, False)
    SetFlag(ui_Button2, lv.obj.FLAG.SCROLL_ON_FOCUS, True)
    SetFlag(ui_Button2, lv.obj.FLAG.CLICKABLE, True)
    ui_Button2.set_style_bg_grad_dir(lv.GRAD_DIR.VER, lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.set_style_bg_grad_color(lv.color_hex(0x616161), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.set_style_bg_color(lv.color_hex(0x616161), lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.set_style_bg_opa(255, lv.PART.MAIN | lv.STATE.PRESSED)
    ui_Button2.add_event_cb(Button2_eventhandler, lv.EVENT.ALL, None)

    ui_Image4 = lv.image(ui_Screen1)
    ui_Image4.set_src(ui_images.ui_img_background1_png)
    # ui_Image4.set_width(lv.SIZE_CONTENT)	# 1
    # ui_Image4.set_height(lv.SIZE_CONTENT)   # 1
    ui_Image4.set_x(-90)
    ui_Image4.set_y(0)
    ui_Image4.set_align(lv.ALIGN.CENTER)
    SetFlag(ui_Image4, lv.obj.FLAG.ADV_HITTEST, True)
    SetFlag(ui_Image4, lv.obj.FLAG.SCROLLABLE, False)

    ui_Label1 = lv.label(ui_Image4)
    ui_Label1.set_text("")
    ui_Label1.set_x(167)
    ui_Label1.set_y(28)
    ui_Label1.set_style_radius(10, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_align(lv.ALIGN.TOP_LEFT)
    ui_Label1.set_style_text_color(lv.color_hex(0xFFFFFF), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_style_text_opa(255, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label1.set_style_text_font(lv.font_montserrat_40, lv.PART.MAIN | lv.STATE.DEFAULT)

    ui_Label2 = lv.label(ui_Image4)
    ui_Label2.set_text("")
    ui_Label2.set_x(175)
    ui_Label2.set_y(180)
    ui_Label2.set_style_radius(10, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_align(lv.ALIGN.TOP_LEFT)
    ui_Label2.set_style_text_color(lv.color_hex(0xFFFFFF), lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_style_text_opa(255, lv.PART.MAIN | lv.STATE.DEFAULT)
    ui_Label2.set_style_text_font(lv.font_montserrat_40, lv.PART.MAIN | lv.STATE.DEFAULT)

    return ui_Screen1, ui_Label1, ui_Label2

def main():
    try:
        # Initialize hardware devices
        display, touch, dht20 = device_init()
        
        # Build UI
        ui_Screen1, ui_Label1, ui_Label2 = create_lvgl_ui()

        LED.value(1)

        lv.screen_load(ui_Screen1)
        
        dht20.measure()
        temp = dht20.temperature()
        hum = dht20.humidity()
        ui_Label1.set_text(f"{round(temp, 1)}")  
        ui_Label2.set_text(f"{round(hum)}")

        # Record the last sensor reading time
        last_dht_time = time.ticks_ms()
        
        print("Initialization completed, entering main loop...")

        # Run LVGL polling event loop
        while True:
            current_time = time.ticks_ms()
            
            # 1. Decouple sensor reading: read sensor every 1000 ms (1 second) without blocking UI
            if time.ticks_diff(current_time, last_dht_time) >= 1000:
                dht20.measure()
                temp = dht20.temperature()
                hum = dht20.humidity()

                ui_Label1.set_text(f"{round(temp, 1)}")  
                ui_Label2.set_text(f"{round(hum)}")

                # print(f"temperature: {round(temp, 1)} C")  
                # print(f"humidity: {round(hum)} %")

                last_dht_time = current_time

            # 2. LVGL heartbeat and event handling (very important)
            lv.tick_inc(5)         # Notify LVGL that 5 ms have passed
            lv.timer_handler()     # Handle redraw and touch events
            
            # 3. Very short sleep to ensure UI frame rate and touch responsiveness
            time.sleep_ms(5)
            
    except Exception as e:
        sys.print_exception(e)

if __name__ == "__main__":
    main()