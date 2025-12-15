# E-Paper Display Layout Overview

## Display Hardware Specifications

- **Display Model**: 7.5 inch E-Paper Display GDEY075T7
- **Physical Resolution**: 800x480 pixels
- **Color Support**: Black & White (2-color), No Gray levels

## Layout Orientations

### Landscape Mode (800x480)

```
┌────────────────────────────────────┬─────────────────────────────────────────┐
│                                    │                                         │
│         WEATHER SECTION            │          DEPARTURE SECTION              │
│           (400x480)                │            (399x480)                    │
│                                    │                                         │
│  • City/Town Name: 22px            │  • Station Name: 17px                   │
│  • Space 20px                      │  • Space 10px                           │
│  • Day weather Info: 80px          │  • Departure Column Headers: 12px       │
│    - first column                  │  • Space 4px                            │
│       - Day Weather Icon: 50px     │  • Line 1px                             │
│        - Current Temp : 30 px      │     Left 421px for depature Entries     │
│    - second column                 │      - Depature Entries 42 px 5 times   │
│       - today low/high temp: 27px  │      - Separation Line 1px              │
│       - UV Index info: 20 px       │      - Depature Entries 42 px 5 times   │
│       - Pollen Info : 20px         │      Single Daparture Entry             │
│    - third column                  │        - Space 3px                      │
│       - Sunrise : 40 px            │        - Main Line: 17px                │
│       - Sunset : 40px              │        - Space 3px                      │
│  • Space 12px                      │        - Disruption Space: 16px         │
│  • Nächste Stunden 15px            │       - Space 3px                       │
│  • Space 25px                      │   • Footer Separation Line  1px         │
│  • Weather Graphic : 304px         │   • Footer: 15px                        │
│  • Space 12px                      │                                         │
│  • Footer: 15px                    │                                         │
└────────────────────────────────────┴─────────────────────────────────────────┘
```

## Update Performance

- **Full Screen**: Complete redraw (~2-3 seconds)
- **Partial Updates**: It is not working with U8g2 font library. It is under development.
