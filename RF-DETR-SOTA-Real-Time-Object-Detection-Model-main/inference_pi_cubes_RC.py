import serial
import time
import cv2
from PIL import Image
from rfdetr import RFDETRBase

CUSTOM_CLASSES = ["placeholder", "BANDAGE", "SYRINGE", "GAUZE"]
model = RFDETRBase(pretrain_weights="cubesmodel/checkpoint_best_total.pth")

# Serial config
ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
time.sleep(2)  # Wait for Arduino
ser.reset_input_buffer()

def fast_resize(image, size=(720, 720)):
    h, w, _ = image.shape
    min_dim = min(h, w)
    x = (w - min_dim) // 2
    y = (h - min_dim) // 2
    return cv2.resize(image[y:y+min_dim, x:x+min_dim], size)

while True:
    line = ser.readline().decode().strip()
    if line == "#DETECT":
        cap = cv2.VideoCapture(0)
        ret, frame = cap.read()
        cap.release()

        if not ret:
            print("Failed to capture image")
            continue

        frame = fast_resize(frame)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image_pil = Image.fromarray(rgb)

        detections = model.predict(image_pil, threshold=0.2)
        class_counts = {"BANDAGE": 0, "SYRINGE": 0, "GAUZE": 0}

        for cid in detections.class_id:
            if 0 < cid < len(CUSTOM_CLASSES):
                cls = CUSTOM_CLASSES[cid]
                class_counts[cls] += 1

        # Format: BANDAGE 1 SYRINGE 0 GAUZE 2
        result = f"BANDAGE {class_counts['BANDAGE']} SYRINGE {class_counts['SYRINGE']} GAUZE {class_counts['GAUZE']}\n"
        ser.write(result.encode())
        print("Sent:", result)