import serial
import os
import time


# Serial port configuration
serial_port = 'COM6'  # Update this with your Arduino Uno serial port
baud_rate = 115200
timeout_seconds = 4

output_directory_text = 'received_texts'
output_directory_image = 'received_images'
output_directory_audio = 'received_audios'

# Create the output directory if it doesn't exist
if not os.path.exists(output_directory_text):
    os.makedirs(output_directory_text)

if not os.path.exists(output_directory_image):
    os.makedirs(output_directory_image)

if not os.path.exists(output_directory_audio):
    os.makedirs(output_directory_audio)

# Counter for naming images
image_counter = 0
audio_counter = 0
text_counter = 0

# Open serial port
SerialObj = serial.Serial(serial_port, baud_rate)
print("Serial port opened")

# i=0
while True:
    # Receive image size
    if SerialObj.in_waiting > 0:
        # Read data from serial port
        data_size_bytes = SerialObj.read(4)  # Assuming image size is sent as 4 bytes (uint32_t)
        data_size = int.from_bytes(data_size_bytes, byteorder='little')
        print("Received size:", data_size)

        j=0
        while j<1:
            if SerialObj.in_waiting > 0:
                data_type_byte = SerialObj.read(1) 
                print(len(data_type_byte))
                j = j+1

        if data_type_byte == b'\x01':
            # Generate filename for the image
            image_filename = os.path.join(output_directory_image, f"image_{image_counter}.jpg")
            image_counter += 1
            # start_time = time.time()

            with open(image_filename, 'wb') as f:
                bytesRemaining = data_size
                while bytesRemaining > 0:
                    inWait = SerialObj.in_waiting
                    # Read data from serial port
                    if inWait > 0: 
                        chunkSize = min(bytesRemaining, inWait)      
                        data = SerialObj.read(chunkSize)  
                        f.write(data)
                        print(len(data))
                        bytesRemaining -= chunkSize                     
                
            print(f'Image received and saved as: {image_filename}')
            # i = i+1

        elif data_type_byte == b'\x02':
            # Generate filename for the image
            audio_filename = os.path.join(output_directory_audio, f"audio_{audio_counter}.mp3")
            audio_counter += 1
            # start_time = time.time()

            with open(audio_filename, 'wb') as f:
                bytesRemaining = data_size
                while bytesRemaining > 0:
                    inWait = SerialObj.in_waiting
                    # Read data from serial port
                    if inWait > 0: 
                        chunkSize = min(bytesRemaining, inWait)      
                        data = SerialObj.read(chunkSize)  
                        f.write(data)
                        bytesRemaining -= chunkSize
                            
            print(f'Audio received and saved as: {audio_filename}')
            # i = i+1

        else:
            # Generate filename for the image
            text_filename = os.path.join(output_directory_text, f"text_{text_counter}.txt")
            text_counter += 1
            # start_time = time.time()

            with open(text_filename, 'wb') as f:
                bytesRemaining = data_size
                while bytesRemaining > 0:
                    inWait = SerialObj.in_waiting
                    # Read data from serial port
                    if inWait > 0: 
                        chunkSize = min(bytesRemaining, inWait)      
                        data = SerialObj.read(chunkSize)  
                        f.write(data)
                        bytesRemaining -= chunkSize
                        
            print(f'Text received and saved as: {text_filename}')
            # i = i+1



SerialObj.close() 
print("Serial port closed")                 
