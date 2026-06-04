# mp3_to_wav.py
# Batch convert MP3 to WAV usable by ESP32 I2S (16kHz, 16-bit, Stereo)
import os
from pydub import AudioSegment

# Input/output folders
input_folder = r"C:\Users\14175\Desktop\convert_wav\Input"
output_folder = r"C:\Users\14175\Desktop\convert_wav\Output"

os.makedirs(output_folder, exist_ok=True)

# Loop through all MP3 files
for filename in os.listdir(input_folder):
    if filename.lower().endswith(".mp3"):
        mp3_path = os.path.join(input_folder, filename)
        wav_filename = os.path.splitext(filename)[0] + ".wav"  # keep original name
        wav_path = os.path.join(output_folder, wav_filename)
        
        # Load MP3
        audio = AudioSegment.from_mp3(mp3_path)
        
        # Check and convert parameters if needed
        needs_conversion = False
        if audio.frame_rate != 16000:
            needs_conversion = True
            audio = audio.set_frame_rate(16000)
        if audio.channels != 2:
            needs_conversion = True
            audio = audio.set_channels(2)
        if audio.sample_width != 2:
            needs_conversion = True
            audio = audio.set_sample_width(2)
        
        # Export WAV
        audio.export(wav_path, format="wav")
        print(f"[OK] {filename} -> {wav_path} (Conversion: {'Yes' if needs_conversion else 'No'})")

print("✅ Batch conversion completed. All files meet ESP32 I2S requirements.")
