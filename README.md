# ESP32-P4 USB Display with Touch Input

This project turns a Waveshare ESP32-P4-Module-DevKit C with a 10.1" display into a second USB monitor for your Windows PC.

## Features

- **USB Video Input**: Receive MJPEG video frames from PC over USB
- **USB Touch Input**: Send touch coordinates back to PC as mouse input
- **10.1" Display Support**: Optimized for Waveshare 10.1" LCD panel
- **Real-time Display**: ~60 FPS display refresh

## Hardware

- Waveshare ESP32-P4-Module-DevKit C
- 10.1" LCD Display (Waveshare)
- GT911 Touch Controller (included with display)
- USB Type-C cable

## Software Requirements

- ESP-IDF v5.0 or later
- Python 3.7+
- USB driver support for ESP32-P4

## Setup Instructions

### 1. Install ESP-IDF

If you haven't already, install ESP-IDF:

```bash
git clone https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
source ./export.sh
```

### 2. Clone this Repository

```bash
git clone https://github.com/gmcknight728-rgb/esp32-p4-usb-display.git
cd esp32-p4-usb-display
```

### 3. Configure and Build

```bash
idf.py set-target esp32p4
idf.py menuconfig  # Optional: adjust settings
idf.py build
```

### 4. Flash to ESP32-P4

Connect your ESP32-P4 to your PC via USB:

```bash
idf.py flash -p /dev/ttyUSB0  # Linux/Mac
idf.py flash -p COM3           # Windows (replace COM3 with your port)
```

To find your COM port on Windows:
- Open Device Manager
- Look for "USB Serial/JTAG" under Ports

### 5. Set Up PC Application

You'll need a PC application to:
1. Capture your desktop
2. Compress frames as MJPEG
3. Send to ESP32 over USB
4. Receive touch input and convert to mouse events

See `pc_app/` for the Windows application.

## Project Structure

```
.
├── main/
│   └── main.c              # Main ESP32 firmware
├── pc_app/
│   ├── screen_capture.py   # Desktop capture & MJPEG encoder
│   └── usb_control.py      # USB communication & input handling
├── CMakeLists.txt
├── idf_component.yml
├── partitions.csv
├── sdkconfig.defaults
└── README.md
```

## Usage

1. **Flash the ESP32-P4** with this firmware
2. **Run the PC application** to start streaming
3. **Touch the display** to control your PC
4. **See your desktop** on the 10.1" screen

## Troubleshooting

### Display shows nothing
- Check LCD connection (SPI pins)
- Verify backlight GPIO (pin 9)
- Check USB serial output for errors

### Touch not working
- Verify I2C connection (pins 6, 7)
- Check GT911 touch controller presence
- Try touch calibration

### Low frame rate
- USB bandwidth may be limited
- Try reducing resolution or increasing compression
- Check USB cable quality

## Performance

- **Resolution**: 1280x800 (10.1")
- **Color Depth**: 16-bit RGB565
- **Target FPS**: 30-60 FPS (depends on USB bandwidth)
- **Latency**: ~100-200ms (USB + processing)

## Notes

This is a work-in-progress project. The PC application is still being developed.

## License

MIT License - See LICENSE file

## Support

For issues or questions, open an issue on GitHub.
