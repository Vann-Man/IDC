import cv2
import numpy as np
from PIL import Image
from rfdetr import RFDETRBase
import supervision as sv
import serial  # Import pyserial
import time

# Initialize serial communication with Arduino
'''
arduino = serial.Serial(port='/dev/tty.usbmodem14101', baudrate=9600, timeout=1)  # Update port as needed
time.sleep(2)  # Wait for the connection to establish
arduino.reset_input_buffer()  # Clear any residual data in the serial buffer
'''
CUSTOM_CLASSES = ["SANDWICH", "WEINER", "BURGER"]

model = RFDETRBase(pretrain_weights="checkpoint_best_ema.pth")

bbox_annotator = sv.BoxAnnotator(color=sv.Color.RED, thickness=2)
label_annotator = sv.LabelAnnotator(
    color=sv.Color.RED,
    text_color=sv.Color.WHITE,
    text_scale=0.5,
    text_thickness=1
)

cap = cv2.VideoCapture(0)  

while True:
    ret, frame = cap.read()
    if not ret:
        break
  
    frame = cv2.resize(frame, (640, 480))
  
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    image_pil = Image.fromarray(frame_rgb)
    
    detections = model.predict(image_pil, threshold=0.5)
    
    labels = []
    for class_id, confidence, bbox in zip(detections.class_id, detections.confidence, detections.xyxy):
        if 0 <= class_id < len(CUSTOM_CLASSES):
            label = f"{CUSTOM_CLASSES[class_id]} {confidence:.2f}"
        else:
            label = f"Unknown {class_id} {confidence:.2f}"
        labels.append(label)
        # Extract bounding box details (x_min, y_min, x_max, y_max)
        x_min, y_min, x_max, y_max = bbox
        width = x_max - x_min
        height = y_max - y_min
        
        # Calculate center coordinates
        center_x = (x_min + x_max) / 2
        center_y = (y_min + y_max) / 2
        
        #send data to arduino
        data_to_send = f"OBJECT,{CUSTOM_CLASSES[class_id]},{width:.2f},{height:.2f},{center_x:.2f},{center_y:.2f}\n"
        # arduino.write(data_to_send.encode())  # Send data as bytes
        print(f"Sent to Arduino: {data_to_send}")

        # adding a circle at the center of object
        cv2.circle(frame, (int(center_x), int(center_y)), radius=10, color=(0, 255, 0), thickness=-1)

    
    
    annotated_image = bbox_annotator.annotate(image_pil.copy(), detections)
    annotated_image = label_annotator.annotate(annotated_image, detections, labels)
    
    annotated_bgr = cv2.cvtColor(np.array(annotated_image), cv2.COLOR_RGB2BGR)
    
    cv2.imshow("rf detr 3numclass", annotated_bgr)
   
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()