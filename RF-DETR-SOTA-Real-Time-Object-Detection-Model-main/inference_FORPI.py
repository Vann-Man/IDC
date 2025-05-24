import cv2
import numpy as np
from PIL import Image
from rfdetr import RFDETRBase
import supervision as sv
import serial  # Import pyserial for Arduino communication
import time
''' REMEMBER TO source/cvenv/bin/activate BEFORE EVERY RUN! '''
# Initialize serial communication with Arduino
'''
arduino = serial.Serial(port='/dev/ttyUSB0', baudrate=9600, timeout=1)  # Update port as needed
time.sleep(2)  # Wait for the connection to establish
arduino.reset_input_buffer()  # Clear any residual data in the serial buffer
'''

# Custom classes and thresholds
CUSTOM_CLASSES = ["placeholder", "BURGER", "SANDWICH", "HOTDOG"]
HOTDOG_CLASS_ID = CUSTOM_CLASSES.index("HOTDOG")
LOW_THRESHOLD = 0.15
HIGH_THRESHOLD = 0.45

TARGET_RESOLUTION = (320, 320)
FRAME_SKIP = 5
FPS_LIMIT = 30

# Load the model
model = RFDETRBase(pretrain_weights="pth14may/checkpoint_best_total.pth")

# Annotators for bounding boxes and labels
bbox_annotator = sv.BoxAnnotator(color=sv.Color.RED, thickness=2)
label_annotator = sv.LabelAnnotator(
    color=sv.Color.RED,
    text_color=sv.Color.WHITE,
    text_scale=0.5,
    text_thickness=1
)

# Camera setup
cap = cv2.VideoCapture(0)
frame_count = 0

# Optimized resizing function
def fast_resize(image, size=(320, 320)):
    h, w, _ = image.shape
    min_dim = min(h, w)
    start_x = (w - min_dim) // 2
    start_y = (h - min_dim) // 2
    cropped = image[start_y:start_y + min_dim, start_x:start_x + min_dim]
    return cv2.resize(cropped, size, interpolation=cv2.INTER_LINEAR)

while True:
    start_time = time.time()
    
    ret, frame = cap.read()
    if not ret:
        break

    frame_count += 1
    if frame_count % FRAME_SKIP != 0:
        continue

    # Preprocess frame using fast_resize
    resized = fast_resize(frame, TARGET_RESOLUTION)
    frame_rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
    image_pil = Image.fromarray(frame_rgb)

    # Run inference
    detections = model.predict(image_pil, threshold=LOW_THRESHOLD)

    # Filter detections with class-specific thresholds
    seen_classes = {}
    for i, (class_id, conf) in enumerate(zip(detections.class_id, detections.confidence)):
        threshold = LOW_THRESHOLD if class_id == HOTDOG_CLASS_ID else HIGH_THRESHOLD
        if conf >= threshold:
            if class_id not in seen_classes or conf > seen_classes[class_id][1]:
                seen_classes[class_id] = (i, conf)

    indices = [idx for idx, _ in seen_classes.values()]
    detections = detections[indices]

    # Prepare labels and send data to Arduino
    labels = []
    for class_id, confidence, bbox in zip(detections.class_id, detections.confidence, detections.xyxy):
        if 0 <= class_id < len(CUSTOM_CLASSES):
            label = f"{CUSTOM_CLASSES[class_id]} {confidence:.2f}"
            # Extract bounding box details (x_min, y_min, x_max, y_max)
            x_min, y_min, x_max, y_max = bbox
            width = x_max - x_min
            height = y_max - y_min
            
            # Calculate center coordinates
            center_x = (x_min + x_max) / 2
            center_y = (y_min + y_max) / 2
            
            # Send data to Arduino
            data_to_send = f"OBJECT,{CUSTOM_CLASSES[class_id]},{width:.2f},{height:.2f},{center_x:.2f},{center_y:.2f}\n"
            # arduino.write(data_to_send.encode())  # Send data as bytes
            print(f"Sent to Arduino: {data_to_send}")

        else:
            label = f"Unknown {class_id} {confidence:.2f}"
        labels.append(label)

    # Annotate the frame
    annotated_image = bbox_annotator.annotate(image_pil.copy(), detections)
    annotated_image = label_annotator.annotate(annotated_image, detections, labels)
    annotated_bgr = cv2.cvtColor(np.array(annotated_image), cv2.COLOR_RGB2BGR)

    # Display the annotated frame
    cv2.imshow("RF-DETR Detection", annotated_bgr)

    # Exit on 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    # FPS control
    elapsed = time.time() - start_time
    time.sleep(max(0, (1 / FPS_LIMIT) - elapsed))

# Release resources
cap.release()
cv2.destroyAllWindows()