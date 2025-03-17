from PIL import Image as PILImage
import numpy as np
import gaussianblur

# This is a simple example of how to use the gaussianblur module in Python.
# The example loads an image, applies a gaussian blur to it, and saves the output image.
def main():
    input_file = "input.png"
    output_file = "output.png"
    # Load image
    pil_img = PILImage.open(input_file)

    # Set the geometry of the image and copy data buffer in the gaussianblur.Image object
    img = gaussianblur.Image()
    img.geom.rows = pil_img.size[1]
    img.geom.cols = pil_img.size[0]
    img.geom.channels = len(pil_img.getbands())
    img.data = np.array(pil_img).flatten().tolist()

    # sigma
    sigma = 7.5
    # apply_to_alpha_channel: If True, the alpha channel will be blurred as well (if present)
    apply_to_alpha_channel = True
    # Apply gaussian blur
    gaussianblur.gaussianblur(img, sigma, apply_to_alpha_channel)
    print("Gaussian blur applied.")

    # Save the output image
    out_np_img = np.array(img.data, dtype=np.uint8).reshape(img.geom.rows, img.geom.cols, img.geom.channels)
    PILImage.fromarray(out_np_img, pil_img.mode).save(output_file)
    print("Output written to", output_file)

if __name__ == "__main__":
    main()