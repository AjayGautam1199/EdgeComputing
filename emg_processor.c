#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOW_SIZE 500
#define MAX_LINE_LEN 64
#define INITIAL_CAPACITY 10000

// Define a structure to hold IIR Biquad Filter state and coefficients
typedef struct {
    double b0, b1, b2, a1, a2;
    double x1, x2, y1, y2;
} BiquadFilter;

// Initialize filter with specific coefficients
void init_filter(BiquadFilter *f, double b0, double b1, double b2, double a1, double a2) {
    f->b0 = b0; f->b1 = b1; f->b2 = b2;
    f->a1 = a1; f->a2 = a2;
    f->x1 = 0; f->x2 = 0;
    f->y1 = 0; f->y2 = 0;
}

// Process a single sample through the filter
double process_filter(BiquadFilter *f, double x) {
    // Standard IIR Difference equation
    double y = (f->b0 * x) + (f->b1 * f->x1) + (f->b2 * f->x2) 
             - (f->a1 * f->y1) - (f->a2 * f->y2);
    
    // Shift state values back in time for the next sample
    f->x2 = f->x1;
    f->x1 = x;
    f->y2 = f->y1;
    f->y1 = y;
    
    return y;
}

int main() {
    // 1. Open the raw data file
    FILE *infile = fopen("raw_emg_data.csv", "r");
    if (!infile) {
        printf("Error: Could not open raw_emg_data.csv\n");
        return 1;
    }

    // Allocate memory for data arrays
    int capacity = INITIAL_CAPACITY;
    long *timestamps = (long *)malloc(capacity * sizeof(long));
    double *raw_emg = (double *)malloc(capacity * sizeof(double));
    double *centered_emg = (double *)malloc(capacity * sizeof(double));
    double *filtered_emg = (double *)malloc(capacity * sizeof(double));
    double *rms_envelope = (double *)malloc(capacity * sizeof(double));
    
    int count = 0;
    double sum_emg = 0.0;
    char line[MAX_LINE_LEN];

    // 2. Read the CSV file line by line
    while (fgets(line, sizeof(line), infile)) {
        if (line[0] == '#' || line[0] == 'T') continue; // Skip headers

        long ts;
        double emg_val;
        if (sscanf(line, "%ld,%lf", &ts, &emg_val) == 2) {
            if (count >= capacity) {
                capacity *= 2;
                timestamps = (long *)realloc(timestamps, capacity * sizeof(long));
                raw_emg = (double *)realloc(raw_emg, capacity * sizeof(double));
                centered_emg = (double *)realloc(centered_emg, capacity * sizeof(double));
                filtered_emg = (double *)realloc(filtered_emg, capacity * sizeof(double));
                rms_envelope = (double *)realloc(rms_envelope, capacity * sizeof(double));
            }
            timestamps[count] = ts;
            raw_emg[count] = emg_val;
            sum_emg += emg_val;
            count++;
        }
    }
    fclose(infile);
    printf("Successfully loaded %d data points.\n", count);

    // 3. Setup the DSP Filters (Calculated specifically for 1000 Hz Sampling Rate)
    BiquadFilter hpf_20hz, lpf_450hz, notch_50hz;
    
    // 2nd-Order Butterworth HPF at 20 Hz
    init_filter(&hpf_20hz, 0.9149691, -1.8299383, 0.9149691, -1.8226949, 0.8371817);
    
    // 2nd-Order Butterworth LPF at 450 Hz
    init_filter(&lpf_450hz, 0.8005924, 1.6011848, 0.8005924, 1.5610181, 0.6413515);
    
    // 2nd-Order Notch Filter at 50 Hz (Q = 30)
    init_filter(&notch_50hz, 0.9947912, -1.8922054, 0.9947912, -1.8922054, 0.9895825);

    double mean_emg = sum_emg / count;
    printf("Calculated Baseline Mean (DC Offset): %.2f\n", mean_emg);

    // 4. Mean-Center AND Apply Filters
    for (int i = 0; i < count; i++) {
        // A) Mean-center the data (subtract baseline offset)
        centered_emg[i] = raw_emg[i] - mean_emg;
        
        // B) Cascade through the filters sequentially
        double val = process_filter(&hpf_20hz, centered_emg[i]);
        val = process_filter(&lpf_450hz, val);
        val = process_filter(&notch_50hz, val);
        
        // Save fully filtered data
        filtered_emg[i] = val;
    }

    // 5. Calculate RMS on the FILTERED data
    for (int i = 0; i < count; i++) {
        double sum_of_squares = 0.0;
        int window_count = 0;

        for (int j = 0; j < WINDOW_SIZE; j++) {
            if (i - j >= 0) {
                // Notice we are now squaring the FILTERED data, not the raw data
                double val = filtered_emg[i - j]; 
                sum_of_squares += (val * val);
                window_count++;
            }
        }
        rms_envelope[i] = sqrt(sum_of_squares / window_count);
    }

    // 6. Write the processed data to a new CSV
    FILE *outfile = fopen("processed_emg_rms.csv", "w");
    if (!outfile) {
        printf("Error: Could not create output file.\n");
        return 1;
    }

    // Notice we are saving the new Filtered_EMG column as well!
    fprintf(outfile, "Timestamp_ms,Centered_EMG,Filtered_EMG,RMS_Envelope\n");
    for (int i = 0; i < count; i++) {
        fprintf(outfile, "%ld,%.4f,%.4f,%.4f\n", timestamps[i], centered_emg[i], filtered_emg[i], rms_envelope[i]);
    }
    fclose(outfile);

    printf("Filtering and RMS complete. Saved to processed_emg_rms.csv\n");

    // Clean up memory
    free(timestamps);
    free(raw_emg);
    free(centered_emg);
    free(filtered_emg);
    free(rms_envelope);

    return 0;
}