import serial
import time
import cv2
from PIL import Image
from rfdetr import RFDETRBase

CUSTOM_CLASSES = ["placeholder", "BANDAGE", "SYRINGE", "GAUZE"]
model = RFDETRBase(pretrain_weights="cubesmodel/checkpoint_best_total.pth")

# Serial config
arduino = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
time.sleep(2)  # Wait for Arduino
arduino.reset_input_buffer()

def fast_resize(image, size=(720, 720)):
    if image is None:
        raise ValueError("Invalid image passed to fast_resize()")
    h, w, _ = image.shape
    min_dim = min(h, w)
    x = (w - min_dim) // 2
    y = (h - min_dim) // 2
    return cv2.resize(image[y:y+min_dim, x:x+min_dim], size)

while True:
    line = arduino.readline().decode().strip()
    if line == "#DETECT":
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print("Error: Unable to access the camera")
            continue

        ret, frame = cap.read()
        if not ret or frame is None:
            print("Error: Failed to capture frame from the camera")
            cap.release()
            continue

        # Display the camera feed
        cv2.imshow("Camera Feed", frame)

        # Resize and preprocess the frame
        frame = fast_resize(frame)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image_pil = Image.fromarray(rgb)

        # Run object detection
        detections = model.predict(image_pil, threshold=0.2)
        class_counts = {"BANDAGE": 0, "SYRINGE": 0, "GAUZE": 0}

        for cid in detections.class_id:
            if 0 < cid < len(CUSTOM_CLASSES):
                cls = CUSTOM_CLASSES[cid]
                class_counts[cls] += 1

        # Format: BANDAGE 1 SYRINGE 0 GAUZE 2
        result = f"BANDAGE {class_counts['BANDAGE']} SYRINGE {class_counts['SYRINGE']} GAUZE {class_counts['GAUZE']}\n"
        arduino.write(result.encode())
        print("Sent:", result)

        # Exit the camera feed on pressing 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

        cap.release()

# Release resources
cap.release()
cv2.destroyAllWindows()