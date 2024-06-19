import socket
import time
import struct

# Define ESP32 IP address and port
esp32_ip = '192.168.4.1'  # Replace with the IP address of your ESP32
esp32_port = 800     # Port number to communicate with the ESP32

# Read audio data from file
with open("audio.mp3", 'rb') as f:
    audio_data = f.read()


audio_size = len(audio_data)

# Convert audio size to uint32_t format
audio_size_bytes = struct.pack('<I', audio_size)
print(len(audio_size_bytes))

# Establish connection with ESP32
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((esp32_ip, esp32_port))

    s.sendall(audio_size_bytes)
    print(f"Sent audio size: {audio_size} bytes")
    time.sleep(2)
    
    # Send audio data in chunks
    chunk_size = 1250  # Adjust chunk size as needed
    total_sent = 0
    while total_sent < len(audio_data):
        chunk = audio_data[total_sent : min(total_sent + chunk_size,len(audio_data))] #min(i+25, 517)
        s.sendall(chunk)
        total_sent += len(chunk)
        print(f"Sent {total_sent} bytes")
        time.sleep(2)

print("Audio data sent successfully")
