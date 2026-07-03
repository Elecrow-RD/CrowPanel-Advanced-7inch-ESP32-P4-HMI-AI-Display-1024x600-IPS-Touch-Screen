import machine
from machine import Pin
import time
import sys
import os
import vfs
import uos

# --------------------------
# ESP32-P4 SD Card Hardware Configuration
# --------------------------
SD_CLK_PIN = 43
SD_CMD_PIN = 44
SD_D0_PIN = 39
SD_POWER_PIN = 38
SD_WIDTH = 1
SD_MOUNT_POINT = "/sd"
def get_file_size_str(size_bytes):
    """Convert bytes to human-readable format"""
    if size_bytes < 0:
        return "Unknown"
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.2f} TB"
def sd_power_on():
    """Turn on SD card power"""
    try:
        power = Pin(SD_POWER_PIN, Pin.OUT, value=1)
        power.value(1)
        time.sleep_ms(200)
        print(f"SD card power ON (GPIO{SD_POWER_PIN})")
        return True
    except Exception as e:
        print(f"SD power control skipped: {e}")
        return True
def init_sd_card_sdio():
    """Initialize SD card using ESP32-P4 SDMMC Slot 0"""
    print("\n" + "=" * 55)
    print("SD Card Initialization (ESP32-P4 SDIO Mode)")
    print("=" * 55)
    print(f"   Chip:   ESP32-P4")
    print(f"   Slot:   SDMMC Slot 0")
    print(f"   CLK:    GPIO{SD_CLK_PIN}")
    print(f"   CMD:    GPIO{SD_CMD_PIN}")
    print(f"   D0:     GPIO{SD_D0_PIN}")
    print(f"   Mode:   {SD_WIDTH}-line SDIO")
    print("-" * 55)
    try:
        os.listdir(SD_MOUNT_POINT)
        print(f"SD card already mounted at {SD_MOUNT_POINT}")
        return True
    except OSError:
        pass
    sd_power_on()
    try:
        sd = machine.SDCard(slot=0, width=SD_WIDTH, freq=20000000)
        vfs.mount(sd, SD_MOUNT_POINT)
        print(f"SD card mounted successfully at {SD_MOUNT_POINT}")
        return True
    except Exception as e:
        sys.print_exception(e)
        print("SDIO mode initialization failed!")
        return False
def list_files_raw(path):
    """
    List files using multiple methods for maximum compatibility
    Handles long filenames (LFN) issues on FAT32
    """
    print("\nFile List (Method 1: os.listdir):")
    print("-" * 55)
    # Method 1: Standard os.listdir
    try:
        entries = os.listdir(path)
        print(f"Found {len(entries)} entries via os.listdir()")
        for entry in sorted(entries):
            print(f"{entry}")
    except OSError as e:
        print(f"os.listdir failed: {e}")
        entries = []
def scan_all_directories(path):
    """
    Recursively scan all directories using os.ilistdir
    More reliable for FAT32 with LFN
    """
    print("\nRecursive Directory Scan:")
    print("=" * 55)
    def scan_dir(current_path, indent=0):
        total_files = 0
        total_bytes = 0
        try:
            for entry in os.ilistdir(current_path):
                name = entry[0]
                entry_type = entry[1]
                # Skip hidden files
                if name.startswith('.'):
                    continue
                full_path = current_path + "/" + name
                is_dir = (entry_type & 0x4000) != 0
                if is_dir:
                    print(" " * indent + f" {name}/")
                    sub_files, sub_bytes = scan_dir(full_path, indent + 4)
                    total_files += sub_files
                    total_bytes += sub_bytes
                else:
                    # Try to get file size
                    try:
                        fstat = os.stat(full_path)
                        fsize = fstat[6]
                        total_files += 1
                        total_bytes += fsize
                        print(" " * indent + f"{name:<31} {get_file_size_str(fsize):>12}")
                    except OSError:
                        print(" " * indent + f"{name}")
                        total_files += 1   
        except OSError as e:
            print(" " * indent + f" Cannot access: {e}")
        return total_files, total_bytes
    total_files, total_bytes = scan_dir(path)
    print("=" * 55)
    print(f"Total: {total_files} files, {get_file_size_str(total_bytes)}")
    return total_files, total_bytes
def get_sd_card_info():
    """Get SD card capacity info using statvfs"""
    try:
        fs_stat = os.statvfs(SD_MOUNT_POINT)
        block_size = fs_stat[0]
        total_blocks = fs_stat[2]
        free_blocks = fs_stat[3]
        total_bytes = block_size * total_blocks
        free_bytes = block_size * free_blocks
        used_bytes = total_bytes - free_bytes
        return total_bytes, free_bytes, used_bytes
    except OSError as e:
        print(f"Failed to get SD card info: {e}")
        return 0, 0, 0
def test_file_read():
    """Test reading a file from SD card"""
    print("\n" + "=" * 55)
    print("File Read Test")
    print("=" * 55)
    # Try to read example.txt if it exists
    test_files = [
        SD_MOUNT_POINT + "/example.txt",
        SD_MOUNT_POINT + "/hello.txt",
        SD_MOUNT_POINT + "/test.txt",
    ]
    for test_file in test_files:
        try:
            print(f"\nTrying to read: {test_file}")
            with open(test_file, "r") as f:
                content = f.read(200)  # Read first 200 chars
            print(f"Success! Content preview:")
            print(f"{content[:100]}...")
            return True
        except OSError:
            pass
    print("Could not read any test file")
    return False
def test_file_write():
    """Test writing a file to SD card"""
    print("\n" + "=" * 55)
    print("File Write Test")
    print("=" * 55)
    test_file = SD_MOUNT_POINT + "/mp_test.txt"
    test_content = "Hello from ESP32-P4 MicroPython!\n"
    test_content += f"Timestamp: {time.localtime()}\n"
    test_content += "MicroPython SD Card Test OK!\n"
    try:
        print(f"\nWriting to: {test_file}")
        with open(test_file, "w") as f:
            f.write(test_content)
        print(" Write successful")
        # Read back
        print(f"Reading back...")
        with open(test_file, "r") as f:
            content = f.read()
        print("Read successful")
        print(f"\nContent:\n{content}")
        # Clean up
        os.remove(test_file)
        print("Test file removed")
        return True
    except OSError as e:
        print(f"File operation failed: {e}")
        return False
def unmount_sd():
    """Unmount SD card"""
    try:
        vfs.umount(SD_MOUNT_POINT)
        print(f"SD card unmounted")
    except OSError as e:
        print(f"Unmount error: {e}")
def main():
    try:
        if not init_sd_card_sdio():
            print("\nProgram exit: SD card init failed")
            return
        list_files_raw(SD_MOUNT_POINT)
        # Capacity info
        print("\nSD Card Capacity:")
        print("-" * 55)
        total, free, used = get_sd_card_info()
        if total > 0:
            print(f"   Total:      {get_file_size_str(total):>15}")
            print(f"   Used:       {get_file_size_str(used):>15}")
            print(f"   Free:       {get_file_size_str(free):>15}")
            print(f"   Usage:      {(used/total*100):>14.1f}%")
        print("=" * 55)
        print("\nDone!")
    except Exception as e:
        sys.print_exception(e)
if __name__ == "__main__":
    main()