# MicroPython: Control Power Amp via STC8 I2C + Low-Noise Audio Playback
# Optimized: Larger buffer, smooth transitions, reduced CPU load

from machine import I2S, Pin, I2C
import time
import math

def log_info(msg):
    print("[INFO]", msg)

def log_error(msg):
    print("[ERROR]", msg)

# ==================== STC8 I2C Config ====================
STC8_I2C_ADDR = 0x2F
STC8_REG_ADDR_SET_GPIO = 0x18
STC8_GPIO_OUT_AUDIO_SD = 2

# I2C pins (modify according to your hardware)
I2C_SCL_PIN = 46
I2C_SDA_PIN = 45

# ==================== I2S Config ====================
AUDIO_GPIO_LRCLK = 21
AUDIO_GPIO_BCLK = 22
AUDIO_GPIO_SDATA = 23
SAMPLE_RATE = 16000

# ==================== Audio Parameters (Optimized)====================
NOTE_FREQ = {
    'C4': 262, 'D4': 294, 'E4': 330, 'F4': 349,
    'G4': 392, 'A4': 440, 'B4': 494,
    'C5': 523, 'D5': 587, 'E5': 659, 'F5': 698, 'G5': 784,
    'REST': 0,
}

SIMPLE_MELODY = [
    ('C4', 2), ('D4', 2), ('E4', 2), ('F4', 2),
    ('G4', 2), ('A4', 2), ('B4', 2), ('C5', 4),
]

BEAT_DURATION = 0.25
CHUNK_SEC = 0.1          # Increased to 0.1s to reduce CPU load
AMPLITUDE = 20000        # Reduced amplitude to minimize distortion

# ==================== Global Variables ====================
i2c_bus = None
tx_i2s = None

# ==================== STC8 I2C Control Functions ====================

def stc8_i2c_init():
    global i2c_bus
    try:
        i2c_bus = I2C(0, scl=Pin(I2C_SCL_PIN), sda=Pin(I2C_SDA_PIN), freq=100000)
        log_info(f"STC8 I2C initialized successfully")
        return True
    except Exception as e:
        log_error(f"STC8 I2C initialization failed: {e}")
        return False

def stc8_gpio_set_level(gpio_num, level):
    global i2c_bus
    if i2c_bus is None:
        return False
    try:
        reg_addr = STC8_REG_ADDR_SET_GPIO + gpio_num
        i2c_bus.writeto_mem(STC8_I2C_ADDR, reg_addr, bytes([level]))
        return True
    except Exception as e:
        log_error(f"STC8 GPIO set failed: {e}")
        return False

def set_audio_ctrl(state):
    """Control audio power amplifier"""
    level = 0 if state else 1
    result = stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD, level)
    if result:
        log_info(f"audio power amplifier {'enabled' if state else 'disabled'}")
    return result

# ==================== I2S Audio Playback Functions ====================

def audio_init():
    """Initialize I2S (optimized buffer config)"""
    global tx_i2s
    try:
        # Increase ibuf to 8192 to reduce underrun
        tx_i2s = I2S(
            1,
            sck=Pin(AUDIO_GPIO_BCLK),
            ws=Pin(AUDIO_GPIO_LRCLK),
            sd=Pin(AUDIO_GPIO_SDATA),
            mode=I2S.TX,
            bits=16,
            format=I2S.STEREO,
            rate=SAMPLE_RATE,
            ibuf=8192  # Increased buffer
        )
        log_info("I2S initialized successfully (ibuf=8192)")
        return True
    except Exception as e:
        log_error(f"I2S initialization failed: {e}")
        return False

def generate_chunk(frequency, chunk_sec, sample_rate, phase_offset):
    """Generate sine wave audio chunk"""
    samples = int(chunk_sec * sample_rate)
    buffer = bytearray(samples * 4)
    amplitude = AMPLITUDE if frequency > 0 else 0
    two_pi = 2 * math.pi

    for i in range(samples):
        if frequency > 0:
            sample = int(amplitude * math.sin(two_pi * frequency * (phase_offset + i) / sample_rate))
        else:
            sample = 0
        left_bytes = (sample & 0xffff).to_bytes(2, 'little')
        right_bytes = (sample & 0xffff).to_bytes(2, 'little')
        idx = i * 4
        buffer[idx:idx+2] = left_bytes
        buffer[idx+2:idx+4] = right_bytes

    return buffer, phase_offset + samples

def play_simple_melody():
    """Play simple melody"""
    log_info("Starting playback...")
    
    events = []
    for note, beats in SIMPLE_MELODY:
        freq = NOTE_FREQ.get(note, 0)
        duration = beats * BEAT_DURATION
        events.append((freq, duration))
    
    current_idx = 0
    event_time = 0.0
    freq, duration = events[0]
    phase = 0
    
    while current_idx < len(events):
        chunk, phase = generate_chunk(freq, CHUNK_SEC, SAMPLE_RATE, phase)
        
        try:
            tx_i2s.write(chunk)
        except Exception as e:
            log_error(f"I2S write failed: {e}")
            break
        
        event_time += CHUNK_SEC
        
        if event_time >= duration:
            current_idx += 1
            if current_idx < len(events):
                freq, duration = events[current_idx]
                event_time = 0.0
    
    log_info("Playback complete")

# ==================== Main Program ====================

def main():
    log_info("=== CrowPanel 5\" ESP32-P4 Audio Test (Optimized) ===")
    
    # 1. Initialize STC8 I2C
    if not stc8_i2c_init():
        return
    
    # 2. Enable audio power amplifier
    if not set_audio_ctrl(True):
        return
    
    time.sleep(0.3)  # Wait for amplifier to stabilize
    
    # 3. Initialize I2S
    if not audio_init():
        set_audio_ctrl(False)
        return
    
    # 4. Play audio
    play_simple_melody()
    
    # 5. Disable power amplifier
    set_audio_ctrl(False)
    
    log_info("Program finished")

if __name__ == "__main__":
    main()