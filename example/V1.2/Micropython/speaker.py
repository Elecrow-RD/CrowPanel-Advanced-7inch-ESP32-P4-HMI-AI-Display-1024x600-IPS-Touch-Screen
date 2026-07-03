# MicroPython Version: Play "Twinkle Twinkle Little Star" (Smooth Edition)
# Board: CrowPanel Advanced 10.1" ESP32-P4
# Full-track continuous streaming, seamless note transitions

from machine import I2S, Pin
import time
import math

def log_info(msg):
    print("[INFO]", msg)

def log_error(msg):
    print("[ERROR]", msg)

# ==================== Note Frequency Table ====================
NOTE_FREQ = {
    'C4': 262, 'D4': 294, 'E4': 330, 'F4': 349,
    'G4': 392, 'A4': 440, 'B4': 494,
    'C5': 523, 'D5': 587, 'E5': 659, 'F5': 698, 'G5': 784,
    'REST': 0,
}

# ==================== Twinkle Twinkle Little Star Melody ====================
MELODY = [
    ('C4', 1), ('C4', 1), ('G4', 1), ('G4', 1),
    ('A4', 1), ('A4', 1), ('G4', 2),
    ('F4', 1), ('F4', 1), ('E4', 1), ('E4', 1),
    ('D4', 1), ('D4', 1), ('C4', 2),
    ('G4', 1), ('G4', 1), ('F4', 1), ('F4', 1),
    ('E4', 1), ('E4', 1), ('D4', 2),
    ('G4', 1), ('G4', 1), ('F4', 1), ('F4', 1),
    ('E4', 1), ('E4', 1), ('D4', 2),
    ('C4', 1), ('C4', 1), ('G4', 1), ('G4', 1),
    ('A4', 1), ('A4', 1), ('G4', 2),
    ('F4', 1), ('F4', 1), ('E4', 1), ('E4', 1),
    ('D4', 1), ('D4', 1), ('C4', 2),
]

BEAT_DURATION = 0.1      # Duration per beat (seconds), adjustable for tempo
CHUNK_SEC = 0.1          # Generate 0.1-second chunks (smaller = smoother but more CPU load)
SAMPLE_RATE = 16000

AUDIO_GPIO_LRCLK = 21
AUDIO_GPIO_BCLK = 22
AUDIO_GPIO_SDATA = 23
AUDIO_GPIO_CTRL = 30

audio_ctrl_pin = Pin(AUDIO_GPIO_CTRL, Pin.OUT)

def set_audio_ctrl(state):
    audio_ctrl_pin.value(not state)

def audio_init():
    global tx_i2s
    tx_i2s = I2S(
        1,
        sck=Pin(AUDIO_GPIO_BCLK),
        ws=Pin(AUDIO_GPIO_LRCLK),
        sd=Pin(AUDIO_GPIO_SDATA),
        mode=I2S.TX,
        bits=16,
        format=I2S.STEREO,
        rate=SAMPLE_RATE,
        ibuf=2048
    )
    return tx_i2s

def generate_chunk(frequency, chunk_sec, sample_rate, phase_offset):
    """Generate a small chunk of sine wave"""
    samples = int(chunk_sec * sample_rate)
    buffer = bytearray(samples * 4)
    amplitude = 22000 if frequency > 0 else 0
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

def play_melody_streaming():
    """Full-track continuous streaming playback (smoothest version)"""
    log_info('Starting "Twinkle Twinkle Little Star" (Smooth Mode)...')

    # Pre-expand all notes into "frequency-duration" sequence
    events = []
    for note, beats in MELODY:
        freq = NOTE_FREQ.get(note, 0)
        duration = beats * BEAT_DURATION
        events.append((freq, duration))

    set_audio_ctrl(True)
    log_info("Amplifier enabled, starting continuous playback...")

    current_event_idx = 0
    event_start_time = 0.0          # Elapsed time for current note
    freq, event_duration = events[0]
    phase = 0

    total_start = time.ticks_ms()

    while current_event_idx < len(events):
        # Calculate frequency for this chunk
        chunk_buffer, phase = generate_chunk(freq, CHUNK_SEC, SAMPLE_RATE, phase)

        # Write to I2S (this is the most time-consuming operation)
        try:
            tx_i2s.write(chunk_buffer)
        except Exception as e:
            log_error(f"I2S write failed: {e}")
            break

        # Update playback progress for current note
        event_start_time += CHUNK_SEC

        # If current note finished, switch to next
        if event_start_time >= event_duration:
            current_event_idx += 1
            if current_event_idx < len(events):
                freq, event_duration = events[current_event_idx]
                event_start_time = 0.0

    elapsed = time.ticks_diff(time.ticks_ms(), total_start) / 1000
    log_info(f"Playback complete! Total duration: {elapsed:.2f} seconds")

    set_audio_ctrl(False)
    log_info("Amplifier disabled")

def main():
    log_info('"Twinkle Twinkle Little Star" Smooth Playback Edition - MicroPython')
    try:
        audio_init()
        set_audio_ctrl(False)
        log_info("I2S initialized successfully")
    except Exception as e:
        log_error(f"I2S initialization failed: {e}")
        return

    play_melody_streaming()

    log_info("Finished, standby mode...")
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()