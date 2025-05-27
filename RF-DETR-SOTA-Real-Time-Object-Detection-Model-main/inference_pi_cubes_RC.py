import serial
import time
import cv2
from PIL import Image
from rfdetr import RFDETRBase

# Define custom classes
CUSTOM_CLASSES = ["placeholder", "BANDAGE", "SYRINGE", "GAUZE"]

# Load the model
print("Loading model...")
model = RFDETRBase(pretrain_weights="cubesmodel/checkpoint_best_total.pth")
print("Model loaded successfully!")

# Initialize serial communication with Arduino
arduino = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
time.sleep(2)  # Wait for Arduino to initialize
arduino.reset_input_buffer()

# Define a function for resizing frames
def fast_resize(image, size=(320, 320)):  # Reduced size for faster processing
    if image is None:
        raise ValueError("Invalid image passed to fast_resize()")
    h, w, _ = image.shape
    min_dim = min(h, w)
    x = (w - min_dim) // 2
    y = (h - min_dim) // 2
    return cv2.resize(image[y:y+min_dim, x:x+min_dim], size)

# Initialize the camera
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("Error: Unable to access the camera")
    exit()

try:
    while True:
        # Read command from Arduino
        line = arduino.readline().decode().strip()
        if line == "#DETECT":
            # Capture a frame from the camera
            ret, frame = cap.read()
            if not ret or frame is None:
                print("Error: Failed to capture frame from the camera")
                continue

            # Resize and preprocess the frame
            resized_frame = fast_resize(frame)
            rgb = cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB)
            image_pil = Image.fromarray(rgb)

            # Display the resized frame
            cv2.imshow("Camera Feed (Resized)", resized_frame)

            # Run object detection
            detections = model.predict(image_pil, threshold=0.2)
            class_counts = {"BANDAGE": 0, "SYRINGE": 0, "GAUZE": 0}

            # Count detected objects
            for cid in detections.class_id:
                if 0 < cid < len(CUSTOM_CLASSES):
                    cls = CUSTOM_CLASSES[cid]
                    class_counts[cls] += 1

            # Format the result string
            result = f"BANDAGE {class_counts['BANDAGE']} SYRINGE {class_counts['SYRINGE']} GAUZE {class_counts['GAUZE']}\n"
            arduino.write(result.encode())
            print("Sent:", result)

        # Ensure the OpenCV window updates
        if cv2.waitKey(1) & 0xFF == ord('q'):  # Exit on pressing 'q'
            break

except KeyboardInterrupt:
    print("Exiting program...")

finally:
    # Release resources
    cap.release()
    cv2.destroyAllWindows()
    arduino.close()