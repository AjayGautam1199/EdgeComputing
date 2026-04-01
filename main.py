import pandas as pd
import matplotlib.pyplot as plt

def main():
    print("Loading processed_emg_rms.csv...")
    try:
        df = pd.read_csv('processed_emg_rms.csv')
    except FileNotFoundError:
        print("Error: processed_emg_rms.csv not found!")
        return

    # Check how many rows were actually loaded
    print(f"Loaded {len(df)} data points from the file.")

    # FIX: Normalize the X-axis!
    # Subtract the very first timestamp from all timestamps, then divide by 1000 
    # to convert milliseconds into standard Seconds (0s, 1s, 2s, 3s...)
    first_timestamp = df['Timestamp_ms'].iloc[0]
    df['Time_Seconds'] = (df['Timestamp_ms'] - first_timestamp) / 1000.0

    # Create a single, large plot window
    plt.figure(figsize=(15, 6))

    # Plot the full raw centered EMG
    plt.plot(df['Time_Seconds'], df['Centered_EMG'], 
             label='Raw Muscle Signal', color='gray', alpha=0.5)
    
    # Plot the full RMS Envelope
    plt.plot(df['Time_Seconds'], df['RMS_Envelope'], 
             label='RMS Envelope', color='red', linewidth=2)
    
    plt.title('Complete EMG Waveform')
    plt.xlabel('Time Elapsed (Seconds)')
    plt.ylabel('Amplitude')
    plt.legend(loc='upper right')
    plt.grid(True, alpha=0.3)

    # Automatically scale the window to fit the whole waveform
    plt.xlim(0, df['Time_Seconds'].iloc[-1])
    plt.tight_layout()

    print("Opening plot window... Use the Magnifying Glass tool at the bottom of the window to zoom in.")
    plt.show()

if __name__ == "__main__":
    main()