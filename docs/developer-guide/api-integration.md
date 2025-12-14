# API Keys Setup

This project requires API keys from external services and uses several free APIs. All keys are stored locally and never
transmitted to third parties.

## Required API Keys

### Overview

- **Google Geolocation API**: Location detection (requires key)
- **RMV Transport API**: German public transport data (requires key)
- **Open-Meteo Weather API**: Weather data via DWD (free, no key)
- **OpenStreetMap Nominatim**: Location names (free, no key)

### 1. Google Geolocation API

**Purpose**: Automatic location detection for finding nearby transport stops.

#### Setup Steps:

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing one
3. Enable the **Geolocation API**
4. Create credentials (API Key)
5. Restrict the API key to Geolocation API only (recommended)

#### Configuration:

```cpp
// src/secrets/google_secrets.h
#pragma once

#define GOOGLE_API_KEY "your_google_api_key_here"
```

#### Cost:

- **Free tier**: 40,000 requests/month
- **Usage**: ~1 request per device setup (one-time)

---

### 2. RMV Transport API

**Purpose**: Real-time public transport departure information for German regional transport.

#### Setup Steps:

1. Visit [RMV Open Data Portal](https://opendata.rmv.de/)
2. Register for an account
3. Request API access for HAFAS API
4. Obtain your API key

#### Configuration:

```cpp
// src/secrets/rmv_secrets.h
#pragma once

#define RMV_API_KEY "your_rmv_api_key_here"
#define RMV_BASE_URL "https://www.rmv.de/hapi/"
```

#### Coverage:

- **Region**: Hesse, Germany (Frankfurt, Wiesbaden, Kassel, etc.)
- **Transport**: Trains, buses, trams, subways
- **Cost**: Free for non-commercial use

---

### 3. Open-Meteo Weather API

**Purpose**: Weather information using DWD (German Weather Service) data.

#### Setup:

**No API key required** - Open-Meteo provides free access to DWD weather data.

#### Configuration:

Already configured in `src/api/dwd_weather_api.cpp`:

```cpp
// No secrets file needed
#define OPEN_METEO_BASE_URL "https://api.open-meteo.com/v1/dwd-icon"
```

#### Coverage:

- **Region**: Germany and surrounding European areas
- **Data**: Temperature, humidity, precipitation, weather conditions, forecasts
- **Cost**: Free (up to 10,000 requests/day)
- **Rate Limits**: 10,000 API calls per day, 5,000 per hour

#### Features:

- Real-time weather data from DWD
- Hourly forecasts up to 7 days
- Historical weather data
- No registration required

---

### 4. OpenStreetMap Nominatim API

**Purpose**: Reverse geocoding to convert coordinates to human-readable location names.

#### Setup:

**No API key required** - Nominatim is free and open source.

#### Configuration:

Already configured in the location detection code:

```cpp
// No secrets file needed
#define NOMINATIM_BASE_URL "https://nominatim.openstreetmap.org/reverse"
```

#### Usage:

- Converts latitude/longitude to city/location names
- Used during initial setup to display location to user
- Provides context for weather and transport data

#### Coverage:

- **Global**: Worldwide coverage from OpenStreetMap data
- **Cost**: Free
- **Rate Limits**: 1 request per second (respected by the device)
- **Usage Policy**: Must include proper attribution, reasonable use

#### Important Notes:

- **Fair Use Policy**: Nominatim is provided as a free service
- **Rate Limiting**: Device automatically respects 1 request/second limit
- **Attribution**: OpenStreetMap data contributors are credited
- **Backup Options**: For high-volume applications, consider self-hosting

---

## API Key Security

### Best Practices

1. **Never commit secrets to Git**
    - Use `.gitignore` to exclude `include/secrets/general_secrets.h` (except examples)
    - Check your repository history for accidentally committed keys

2. **Restrict API key permissions**
    - Google: Limit to Geolocation API only
    - RMV: Use for personal/development purposes only

3. **Monitor usage**
    - Check Google Cloud Console for unexpected usage
    - Monitor RMV API quotas

## Other Transport APIs

According to recent reports, RMV and NRW public transport systems have been experiencing high delays and cancellations:
https://www.adac.de/news/test-oepnv-ausfaelle-verspaetungen/
NRW API implementation will be followed up in future releases.

### Germany

- **DB API** - Deutsche Bahn (national railways) : cost near 4000 Euros, to get one API access (State 2025)
- **RMV API** - Rhein-Main transport (default) : 67% S-Bahn on-time rate (less than 3 minutes delay)
- **NRW API** - North Rhine-Westphalia transport : Köln, Düsseldorf, Dortmund
- **VBB API** - Berlin-Brandenburg transport
- **MVV API** - Munich transport
- **HVV API** - Hamburg transport : over 95% high on-time rate in public transport, may not need to implement

### International

- **GTFS** - General Transit Feed Specification (worldwide)
- **TfL API** - Transport for London
- **RATP API** - Paris transport
- **SL API** - Stockholm transport

### Adapting for Other APIs

1. Create new API files in `src/api/`
2. Implement similar functions to `rmv_api.cpp`
3. Update configuration to use new API
4. Add new secrets file if required

## Next Steps

After setting up API keys:

- [Quick Start Guide](./quick-start.md) - Initial device configuration
- [Configuration Guide](./configuration.md) - Detailed setup options
