# CSV Data Visualization Examples

This directory contains example CSV datasets that can be visualized using the `csv_demo` program.

## Usage

```bash
# Build the CSV demo
make csv_demo

# Run with default dataset
./csv_demo

# Run with specific dataset
./csv_demo data/cpu_usage.csv
./csv_demo data/network_traffic.csv
```

## CSV Format Requirements

The CSV file must have the following:
- **Header row** with column names
- **Required columns:** `x`, `y` (for particle positions)
- **Optional columns:** `speed` (for velocity), `value` (for color mapping)

### Example Format

```csv
x,y,speed,value
10,5,15,100
20,8,12,85
15,12,18,95
...
```

## Column Mapping

The CSV loader maps columns to particle properties as follows:

| CSV Column | Particle Property | Description |
|------------|------------------|-------------|
| `x` | X Position | Horizontal position (required) |
| `y` | Y Position | Vertical position (required) |
| `speed` | Velocity | Particle movement speed (optional) |
| `value` | Color | Mapped to color gradient (optional) |

## Color Gradient

The `value` column is mapped to a color gradient:
- **Low values (min):** Blue
- **Mid values:** Green
- **High values (max):** Red

The gradient automatically scales to the min/max values in your dataset.

## Example Datasets

### 1. example.csv
**Purpose:** Basic demonstration dataset
**Rows:** 20
**Columns:** x, y, speed, value
**Description:** Random data points for testing

### 2. cpu_usage.csv
**Purpose:** Simulated CPU usage across multiple cores
**Rows:** 32
**Columns:** x (time), y (core), speed (activity), value (CPU %)
**Description:** Visualize CPU usage patterns over time

### 3. network_traffic.csv
**Purpose:** Simulated network packet data
**Rows:** 20
**Columns:** x (time), y (destination), speed (bandwidth), value (packet size)
**Description:** Visualize network traffic flow

## Creating Your Own Datasets

You can create custom CSV files to visualize any data:

```csv
x,y,speed,value
<x_pos>,<y_pos>,<velocity>,<color_value>
...
```

### Tips for Good Visualizations

1. **Position Range:** Keep x and y values within your terminal size (typically 0-80 for x, 0-40 for y)
2. **Speed Values:** Use 0-20 for reasonable particle motion
3. **Value Range:** Can be any numerical range - will auto-scale to colors
4. **Data Density:** 20-100 data points work well for terminal visualization

## Interactive Controls

While the visualization is running:
- **q** - Quit visualization
- Particles will gently drift with physics enabled
- Colors automatically map to your value range

## Examples of Real-World Data

This CSV loader can visualize:
- **System metrics** (CPU, memory, disk I/O over time)
- **Sensor data** (temperature, pressure, humidity readings)
- **Network activity** (packet flows, connection patterns)
- **Financial data** (stock prices, trading volume)
- **Scientific data** (experiment results, measurements)
- **Game analytics** (player positions, scores, events)

## Advanced Usage

### Preprocessing Your Data

If your data isn't in the right format, you can preprocess it:

```bash
# Example: Convert JSON to CSV using jq
cat data.json | jq -r '.[] | [.x, .y, .speed, .value] | @csv' > data.csv

# Example: Extract specific columns from larger CSV
cut -d',' -f1,2,5,8 input.csv > formatted.csv
```

### Scaling Data

If your data values are too large or small:

```python
# Python script to scale data
import pandas as pd

df = pd.read_csv('input.csv')
df['x'] = (df['x'] - df['x'].min()) / (df['x'].max() - df['x'].min()) * 80
df['y'] = (df['y'] - df['y'].min()) / (df['y'].max() - df['y'].min()) * 40
df.to_csv('scaled.csv', index=False)
```

## Future Enhancements

The full Week 1 implementation will add:
- JSON format support
- Real-time streaming data (pipes, sockets)
- Configuration files for column mapping
- Multiple visualization modes
- Data filtering and transformation
- Plugin architecture for custom data sources
