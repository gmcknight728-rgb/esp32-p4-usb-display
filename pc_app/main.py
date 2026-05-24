"""
ESP32-P4 USB Display - Windows PC Application
Captures desktop and streams to ESP32-P4 over USB
"""

import serial
import cv2
import numpy as np
import threading
import time
from PIL import ImageGrab
import struct
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class ESP32Display:
    def __init__(self, port='COM8', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.running = False
        self.frame_count = 0
        
        # Display settings
        self.display_width = 1280
        self.display_height = 800
        self.fps = 30
        self.quality = 80  # JPEG quality (1-100)
        
    def connect(self):
        """Connect to ESP32-P4 via USB serial"""
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=1)
            logger.info(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            logger.error(f"Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from ESP32"""
        if self.serial_conn:
            self.serial_conn.close()
            logger.info("Disconnected")
    
    def capture_screen(self):
        """Capture the screen"""
        try:
            # Capture full screen
            screenshot = ImageGrab.grab()
            
            # Convert to OpenCV format (BGR)
            frame = cv2.cvtColor(np.array(screenshot), cv2.COLOR_RGB2BGR)
            
            # Resize to display resolution
            frame = cv2.resize(frame, (self.display_width, self.display_height))
            
            return frame
        except Exception as e:
            logger.error(f"Error capturing screen: {e}")
            return None
    
    def compress_frame(self, frame):
        """Compress frame as JPEG"""
        try:
            ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, self.quality])
            if ret:
                return buffer.tobytes()
            else:
                logger.error("Failed to encode frame")
                return None
        except Exception as e:
            logger.error(f"Error compressing frame: {e}")
            return None
    
    def send_frame(self, frame_data):
        """Send frame to ESP32"""
        if not self.serial_conn or not frame_data:
            return False
        
        try:
            # Frame format: [HEADER(4 bytes) | SIZE(4 bytes) | DATA]
            header = b'FRAM'
            size = len(frame_data)
            
            # Send header
            self.serial_conn.write(header)
            time.sleep(0.01)
            
            # Send size (4 bytes, little endian)
            self.serial_conn.write(struct.pack('<I', size))
            time.sleep(0.01)
            
            # Send frame data in chunks
            chunk_size = 1024
            for i in range(0, size, chunk_size):
                chunk = frame_data[i:i+chunk_size]
                self.serial_conn.write(chunk)
                time.sleep(0.005)
            
            self.frame_count += 1
            return True
        except Exception as e:
            logger.error(f"Error sending frame: {e}")
            return False
    
    def read_touch_input(self):
        """Read touch input from ESP32"""
        if not self.serial_conn:
            return None
        
        try:
            if self.serial_conn.in_waiting > 0:
                data = self.serial_conn.read(self.serial_conn.in_waiting)
                # Parse touch data here
                return data
        except Exception as e:
            logger.error(f"Error reading input: {e}")
        
        return None
    
    def stream_loop(self):
        """Main streaming loop"""
        logger.info("Starting screen capture stream...")
        self.running = True
        
        frame_time = 1.0 / self.fps
        last_time = time.time()
        
        while self.running:
            try:
                current_time = time.time()
                elapsed = current_time - last_time
                
                # Capture and send frame at target FPS
                if elapsed >= frame_time:
                    frame = self.capture_screen()
                    if frame is not None:
                        compressed = self.compress_frame(frame)
                        if compressed:
                            self.send_frame(compressed)
                            logger.info(f"Sent frame {self.frame_count} ({len(compressed)} bytes)")
                    
                    last_time = current_time
                
                # Check for touch input
                touch_data = self.read_touch_input()
                if touch_data:
                    logger.debug(f"Received touch input: {touch_data}")
                
                time.sleep(0.001)
            
            except KeyboardInterrupt:
                logger.info("Interrupted by user")
                break
            except Exception as e:
                logger.error(f"Error in stream loop: {e}")
                time.sleep(0.1)
        
        self.running = False
        logger.info(f"Stream stopped. Total frames sent: {self.frame_count}")
    
    def start(self):
        """Start the application"""
        if not self.connect():
            return
        
        try:
            self.stream_loop()
        finally:
            self.disconnect()


def main():
    """Main entry point"""
    logger.info("ESP32-P4 USB Display Application")
    logger.info("================================")
    
    # Configuration
    port = 'COM8'  # Change this to your ESP32's COM port
    
    # Create and start display
    display = ESP32Display(port=port)
    display.start()


if __name__ == '__main__':
    main()
