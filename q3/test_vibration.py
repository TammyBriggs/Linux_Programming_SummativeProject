import vibration

def main():
    print("--- Testing Industrial Vibration C Extension ---")
    
    # Sample vibration data (m/s²)
    data = [2.5, -1.2, 3.8, 0.4, -2.1, 4.5, -0.8]
    threshold = 2.0

    print(f"\nData: {data}")
    print(f"Threshold: {threshold}")
    
    print("\n--- Computations ---")
    print(f"Peak-to-Peak:    {vibration.peak_to_peak(data):.4f}")
    print(f"RMS:             {vibration.rms(data):.4f}")
    print(f"Standard Dev:    {vibration.std_dev(data):.4f}")
    print(f"Above Threshold: {vibration.above_threshold(data, threshold)}")
    
    summary_dict = vibration.summary(data)
    print("\n--- Summary Dictionary ---")
    for key, val in summary_dict.items():
        print(f"  {key.capitalize()}: {val:.4f}" if isinstance(val, float) else f"  {key.capitalize()}: {val}")

    print("\n--- Testing Edge Cases ---")
    
    # Edge Case 1: Empty List
    try:
        print("Testing empty list... ", end="")
        vibration.rms([])
    except ValueError as e:
        print(f"Caught safely! -> {e}")

    # Edge Case 2: Wrong Type
    try:
        print("Testing string input... ", end="")
        vibration.summary("not a list")
    except TypeError as e:
        print(f"Caught safely! -> {e}")

if __name__ == "__main__":
    main()
