import cv2
import numpy as np
from PIL import Image
from rfdetr import RFDETRBase
import supervision as sv
import time

CUSTOM_CLASSES = ["placeholder","BURGER","SANDWICH","HOTDOG"]
TARGET_RESOLUTION = (720, 720)
FRAME_SKIP = 3
FPS_LIMIT = 30

model = RFDETRBase(pretrain_weights="pth14may/checkpoint_best_total.pth")

bbox_annotator = sv.BoxAnnotator(color=sv.Color.RED, thickness=2)
label_annotator = sv.LabelAnnotator(
    color=sv.Color.RED,
    text_color=sv.Color.WHITE,
    text_scale=0.5,
    text_thickness=1
)

cap = cv2.VideoCapture(1)
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

    
    letterboxed, scale, pad_x, pad_y = letterbox(frame, TARGET_RESOLUTION)
    frame_rgb = cv2.cvtColor(letterboxed, cv2.COLOR_BGR2RGB)
    image_pil = Image.fromarray(frame_rgb)

    
    detections = model.predict(image_pil, threshold=0.5)

    
    seen_classes = {}
    for i, (class_id, conf) in enumerate(zip(detections.class_id, detections.confidence)):
        if class_id not in seen_classes or conf > seen_classes[class_id][1]:
            seen_classes[class_id] = (i, conf)
    indices_to_keep = [idx for idx, _ in seen_classes.values()]
    detections = detections[indices_to_keep]

    
    labels = []
    for class_id, confidence in zip(detections.class_id, detections.confidence):
        if 0 <= class_id < len(CUSTOM_CLASSES):
            labels.append(f"{CUSTOM_CLASSES[class_id]} {confidence:.2f}")
        else:
            labels.append(f"Unknown {class_id} {confidence:.2f}")

    
    annotated_image = bbox_annotator.annotate(image_pil.copy(), detections)
    annotated_image = label_annotator.annotate(annotated_image, detections, labels)
    annotated_bgr = cv2.cvtColor(np.array(annotated_image), cv2.COLOR_RGB2BGR)

    
    cv2.imshow("RF-DETR Detection", annotated_bgr)

    
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    
    elapsed = time.time() - start_time
    time.sleep(max(0, (1 / FPS_LIMIT) - elapsed))

cap.release()
cv2.destroyAllWindows()