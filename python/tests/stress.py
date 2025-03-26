import time
import numpy as np
import gaussianblur

def test_stress_gaussian_blur():
    # Define image dimensions and channels (10240x10240 RGBA)
    width = 10240
    height = 10240
    channels = 4

    # Create an ImgGeom instance and set its attributes
    image = gaussianblur.Image()
    image.geom.rows = height
    image.geom.cols = width
    image.geom.channels = channels
    image.data = np.random.randint(0, 256, size=(width * height * channels), dtype=np.uint8).tolist()

    # Convert image data to a NumPy array for variance calculation
    np_image = np.array(image.data, dtype=np.uint8).reshape((height, width, channels))
    initial_variance = np.var(np_image)

    # Test Gaussian blur with different sigma values
    sigma_values = [25.0, 50.0, 100.0, 150.0]
    durations = []

    for sigma in sigma_values:
        # Measure the time taken to apply Gaussian blur
        start = time.time()
        gaussianblur.gaussianblur(image, sigma, apply_to_alpha=True)
        duration = time.time() - start
        durations.append(duration)
        print(f"Sigma: {sigma}, Duration: {duration:.4f} seconds.")

    # Assert that the processing times are consistent across sigma values
    max_duration = max(durations)
    assert max_duration < 10.0, "Processing time exceeds 10 seconds for at least one sigma value."
    min_duration = min(durations)
    print(f"Max duration: {max_duration:.4f}, Min duration: {min_duration:.4f}")
    assert max_duration - min_duration < 2.0, "Processing time varies significantly across sigma values."

    # Convert blurred image data back to a NumPy array
    blurred_np_image = np.array(image.data, dtype=np.uint8).reshape((height, width, channels))
    blurred_variance = np.var(blurred_np_image)

    print(f"Initial variance: {initial_variance:.4f}, Blurred variance: {blurred_variance:.4f}")

    # Assert that the image has been blurred (variance should decrease)
    assert blurred_variance < initial_variance, "Image variance did not decrease after applying Gaussian blur."