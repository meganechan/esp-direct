# SimHub Setup Instructions for ESP32 Racing Dashboard

## Serial Port Settings
- **Baud Rate**: 115200
- **Update Frequency**: 30-60 FPS (recommended)

## Custom Serial Format
ใช้ format string นี้ใน SimHub Custom Serial Device:

```
'S:' + [DataCorePlugin.GameData.SpeedKmh] + ';R:' + (round([DataCorePlugin.GameData.Rpms] / 100,0) * 100) + ';MR:' + [DataCorePlugin.GameData.MaxRpm] + ';F:' + format([DataCorePlugin.GameData.FuelPercent] * [DataCorePlugin.GameData.MaxFuel] / 100, '0.0') + ';G:' + [DataCorePlugin.GameData.Gear] + ';L:' + format([DataCorePlugin.GameData.LastLapTime],'mm\\:ss\\.fff') + ';N:' + format([DataCorePlugin.GameData.BestLapTime],'mm\\:ss\\.fff') + ';\n'
```

## Data Format Explanation

| Field | Description | Example | Unit |
|-------|-------------|---------|------|
| S: | Speed in km/h | `S:120` | km/h |
| R: | RPM (rounded to nearest 100) | `R:6500` | RPM |
| MR: | Maximum RPM for current car | `MR:8000` | RPM |
| F: | Fuel amount in liters | `F:45.5` | Liters |
| G: | Current gear | `G:4` or `G:N` or `G:R` | - |
| L: | Last lap time | `L:01:23.456` | mm:ss.fff |
| N: | Best lap time | `N:01:20.123` | mm:ss.fff |

## Example Data String
```
S:120;R:6500;MR:8000;F:45.5;G:4;L:01:23.456;N:01:20.123;
```

## SimHub Configuration Steps
1. Open SimHub
2. Go to "Arduino/ESP32" tab
3. Select your COM port
4. Set baud rate to 115200
5. Paste the format string above into the "Custom protocol" field
6. Enable the device
7. Your ESP32 dashboard should start receiving data

## Troubleshooting
- Make sure COM port is correct
- Check baud rate matches (115200)
- Verify ESP32 is connected and powered
- Check serial monitor for incoming data
- Ensure no other program is using the serial port 