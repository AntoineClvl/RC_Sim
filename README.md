# RC car controlled by race sim

## What is it ?

This project comes from an old idea of controlling a car not from its controller, but from a steering wheel and pedals connected to a computer. In addition to that, I planned to implement a camera in the car to have live feedback on the screen.

## How ?

By collecting inputs from a Thrustmaster T300 with DirectInput via a C++ backend, processing the data and sending it via serial communication to an ESP32. This ESP32 will then use a NRF24L01 module to send forward the data to a second ESP32 located in the RC car to control the steering servomotor and the ESC.
By placing an FPV camera with integrated antenna and connecting an Eachine ROTG02 to the computer, it is possible to get the camera feedback in real-time on screen and controlling the car from a racing simulator.

## System Architecture

The system is split into three main components:

1. **PC (C++ backend)**
   - Reads steering wheel and pedal inputs via DirectInput
   - Normalizes and filters inputs
   - Sends control packets over USB serial

2. **Transmitter ESP32**
   - Receives packets via serial
   - Validates packet integrity (SOF/EOF, length, checksum)
   - Transmits data wirelessly using an nRF24L01 module

3. **Receiver ESP32 (RC Car)**
   - Receives and validates RF packets
   - Controls the steering servo and ESC
   - Handles failsafe behavior in case of signal loss

## Communication Protocol

A custom binary packet protocol is used between the PC and the RC car:

- Start of frame (SOF)
- Payload length
- Steering value (int32)
- Throttle value (int32)
- End of frame (EOF)
- Checksum (XOR)

The protocol is designed to:
- Be compact and fast
- Allow resynchronization on corrupted data
- Detect transmission errors both over serial and RF

## Latency & Performance

Low latency is critical for realistic driving control.

Key optimizations:
- Fixed-size RF payloads for deterministic transmission time
- Lightweight XOR checksum at application level
- Non-blocking serial and RF handling
- Disabled auto-acknowledgment to reduce transmission delay

Typical end-to-end latency (wheel → RC car) is on the order of a few milliseconds, enabling precise real-time control.

## Safety & Failsafe

To prevent unintended behavior:
- Packets are validated before being applied
- Invalid or corrupted packets are ignored
- Neutral throttle is enforced when no valid packet is received
- Steering and throttle values are constrained to safe ranges

These measures help protect both the hardware and the surroundings.

## Hardware

- Thrustmaster T300 steering wheel and pedals
- ESP32 (x2)
- nRF24L01 2.4 GHz transceivers
- RC car with steering servo and ESC
- FPV camera with integrated antenna
- Eachine ROTG02 USB receiver

## Software & Tools

- C++ (DirectInput, serial communication, OpenCV)
- Arduino framework (ESP32)
- RF24 library
- Git & GitHub

## Future Improvements

- Bidirectional RF communication (telemetry)
- IMU-based gyroscope for drifting abilities
- Adaptive steering filtering based on vehicle speed
- Improved RF protocol with acknowledgments and packet loss detection
- Integration of video and control data into a single interface
- Use of other microcontroller and RF module for better capabilities and responsiveness









