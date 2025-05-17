import cv2
import numpy as np
from PIL import Image
from rfdetr import RFDETRBase
import supervision as sv
import serial  # Import pyserial for Arduino communication
import time


''' REMEMBER TO source cvenv/bin/activate ON EVERY RUN '''

# Initialize serial communication with Arduino
#arduino = serial.Serial(port='/dev/ttyUSB0', baudrate=9600, timeout=1)  # Update port as needed
#time.sleep(2)  # Wait for the connection to establish
#arduino.reset_input_buffer()  # Clear any residual data in the serial buffer

CUSTOM_CLASSES = ["placeholder", "BURGER", "SANDWICH", "HOTDOG"]
TARGET_RESOLUTION = (720, 720)
FRAME_SKIP = 5
FPS_LIMIT = 30

model = RFDETRBase(pretrain_weights="pth14may/checkpoint_best_total.pth")

bbox_annotator = sv.BoxAnnotator(color=sv.Color.RED, thickness=2)
label_annotator = sv.LabelAnnotator(
    color=sv.Color.RED,
    text_color=sv.Color.WHITE,
    text_scale=0.5,
    text_thickness=1
)

cap = cv2.VideoCapture(0)
frame_count = 0

def letterbox(image, target_size=(416, 416), color=(114, 114, 114)):
    h, w = image.shape[:2]
    tw, th = target_size
    scale = min(tw / w, th / h)
    nw, nh = int(w * scale), int(h * scale)
    resized = cv2.resize(image, (nw, nh), interpolation=cv2.INTER_LINEAR)
    canvas = np.full((th, tw, 3), color, dtype=np.uint8)
    top, left = (th - nh) // 2, (tw - nw) // 2
    canvas[top:top + nh, left:left + nw] = resized
    return canvas, scale, left, top

while True:
    start_time = time.time()
    
    ret, frame = cap.read()
    if not ret:
        break

    frame_count += 1
    if frame_count % FRAME_SKIP != 0:
        continue

    # Preprocess frame
    letterboxed, scale, pad_x, pad_y = letterbox(frame, TARGET_RESOLUTION)
    frame_rgb = cv2.cvtColor(letterboxed, cv2.COLOR_BGR2RGB)
    image_pil = Image.fromarray(frame_rgb)

    # Run inference
    detections = model.predict(image_pil, threshold=0.5)

    # Filter detections
    seen_classes = {}
    for i, (class_id, conf) in enumerate(zip(detections.class_id, detections.confidence)):
        if class_id not in seen_classes or conf > seen_classes[class_id][1]:
            seen_classes[class_id] = (i, conf)
    indices_to_keep = range(len(detections.class_id))  # Keep all indices
    detections = detections[indices_to_keep]

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