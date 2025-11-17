#!/usr/bin/env python3
import csv
import random
import math
from datetime import datetime, timedelta

def generate_ohlcv_data(symbol, num_bars=1000, start_price=100.0):
    """Generate synthetic OHLCV data for testing"""
    
    data = []
    current_price = start_price
    start_time = datetime(2023, 1, 1, 9, 30)
    
    for i in range(num_bars):
        timestamp = int((start_time + timedelta(minutes=i)).timestamp() * 1e9)
        
        # Random walk with trend
        change = random.gauss(0, 0.02) + math.sin(i / 50) * 0.01
        current_price *= (1 + change)
        
        # Generate OHLC
        open_price = current_price
        high = open_price * (1 + abs(random.gauss(0, 0.01)))
        low = open_price * (1 - abs(random.gauss(0, 0.01)))
        close = random.uniform(low, high)
        volume = random.randint(100000, 1000000)
        
        data.append({
            'timestamp': timestamp,
            'open': round(open_price, 2),
            'high': round(high, 2),
            'low': round(low, 2),
            'close': round(close, 2),
            'volume': volume
        })
        
        current_price = close
    
    return data

def write_csv(filename, data):
    """Write data to CSV file"""
    with open(filename, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['timestamp', 'open', 'high', 'low', 'close', 'volume'])
        writer.writeheader()
        writer.writerows(data)

if __name__ == '__main__':
    symbols = ['AAPL', 'GOOGL', 'MSFT', 'TSLA']
    
    for symbol in symbols:
        data = generate_ohlcv_data(symbol, num_bars=2000)
        filename = f'data/historical/{symbol}.csv'
        write_csv(filename, data)
        print(f'Generated {filename}')
