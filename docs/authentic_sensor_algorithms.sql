-- Authentic Derived Sensor Algorithms
--
-- This script replaces the original naive algorithms for Heat Index and Dew Point
-- in the sensor_readings table. It implements:
-- 1. Full NOAA / Adafruit Heat Index algorithm (including Steadman's relaxed formula)
-- 2. Arden Buck Equation for Dew Point (highly accurate for typical ambient conditions)
--

-- Update Heat Index
CREATE OR REPLACE FUNCTION calculate_heat_index(temp_c NUMERIC, humidity NUMERIC)
RETURNS NUMERIC AS $$
DECLARE
  temp_f NUMERIC;
  hi NUMERIC;
BEGIN
  IF humidity <= 0 THEN RETURN temp_c; END IF;

  -- Convert C to F
  temp_f := (temp_c * 1.8) + 32.0;

  -- Steadman's simple formula (valid for lower temps)
  hi := 0.5 * (temp_f + 61.0 + ((temp_f - 68.0) * 1.2) + (humidity * 0.094));

  -- If the simple heat index is high enough, we use the Rothfusz regression
  IF hi > 79.0 THEN
    hi := -42.379 + 2.04901523 * temp_f + 10.14333127 * humidity 
         - 0.22475541 * temp_f * humidity 
         - 0.00683783 * (temp_f * temp_f) 
         - 0.05481717 * (humidity * humidity) 
         + 0.00122874 * (temp_f * temp_f) * humidity 
         + 0.00085282 * temp_f * (humidity * humidity) 
         - 0.00000199 * (temp_f * temp_f) * (humidity * humidity);

    -- Adjustments
    IF humidity < 13.0 AND temp_f >= 80.0 AND temp_f <= 112.0 THEN
      hi := hi - ((13.0 - humidity) * 0.25) * sqrt((17.0 - abs(temp_f - 95.0)) * 0.058823529);
    ELSIF humidity > 85.0 AND temp_f >= 80.0 AND temp_f <= 87.0 THEN
      hi := hi + ((humidity - 85.0) * 0.1) * ((87.0 - temp_f) * 0.2);
    END IF;
  END IF;

  -- Convert back to Celsius
  RETURN ROUND(((hi - 32.0) / 1.8), 2);
END;
$$ LANGUAGE plpgsql;


-- Update Dew Point
-- Valid for -80 to 50 C
CREATE OR REPLACE FUNCTION calculate_dew_point(temp_c NUMERIC, humidity NUMERIC)
RETURNS NUMERIC AS $$
DECLARE
  b NUMERIC;
  c NUMERIC;
  d NUMERIC;
  gamma NUMERIC;
BEGIN
  IF humidity <= 0 OR temp_c IS NULL THEN RETURN NULL; END IF;
  
  -- Arden Buck constants for > 0 C and < 0 C
  IF temp_c > 0 THEN
    b := 17.368;
    c := 238.88;
  ELSE
    b := 17.856;
    c := 245.52;
  END IF;

  IF humidity = 0 THEN
      RETURN -273.15;
  END IF;

  gamma := LN(humidity / 100.0) + (b * temp_c) / (c + temp_c);
  
  -- DP = (c * gamma) / (b - gamma)
  RETURN ROUND((c * gamma) / (b - gamma), 2);
END;
$$ LANGUAGE plpgsql;
