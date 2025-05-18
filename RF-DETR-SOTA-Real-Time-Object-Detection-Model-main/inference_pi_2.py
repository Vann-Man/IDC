import cv2
import numpy as np
import threading
from PIL import Image
from rfdetr import RFDETRBase
import supervision as sv

CUSTOM_CLASSES = ["placeholder", "BURGER", "SANDWICH", "HOTDOG"]
HOTDOG_CLASS_ID = CUSTOM_CLASSES.index("HOTDOG")
LOW_THRESHOLD = 0.15
HIGH_THRESHOLD = 0.45

TARGET_RESOLUTION = (224, 224)

model = RFDETRBase(pretrain_weights="pth14may/checkpoint_best_total.pth")

bbox_annotator = sv.BoxAnnotator(color=sv.Color.RED, thickness=2)
label_annotator = sv.LabelAnnotator(
    color=sv.Color.RED,
    text_color=sv.Color.WHITE,
    text_scale=0.5,
    text_thickness=1
)

class CameraStream:
    def __init__(self, src=0):
        self.cap = cv2.VideoCapture(src)
        self.ret, self.frame = self.cap.read()
        self.running = True
        threading.Thread(target=self.update, daemon=True).start()

    def update(self):
        while self.running:
            self.ret, self.frame = self.cap.read()

    def read(self):
        return self.ret, self.frame

    def release(self):
        self.running = False
        self.cap.release()

def fast_resize(image, size=(224, 224)):
    h, w, _ = image.shape
    min_dim = min(h, w)
    
    start_x = (w - min_dim) // 2
    start_y = (h - min_dim) // 2
    cropped = image[start_y:start_y + min_dim, start_x:start_x + min_dim]
    
    return cv2.resize(cropped, size, interpolation=cv2.INTER_LINEAR)

camera = CameraStream()

while True:
    ret, frame = camera.read()
    if not ret:
        break

    resized = fast_resize(frame)
    rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
    image_pil = Image.fromarray(rgb)

    
    detections = model.predict(image_pil, threshold=LOW_THRESHOLD)

    
    seen_classes = {}
    for i, (class_id, conf) in enumerate(zip(detections.class_id, detections.confidence)):
        threshold = LOW_THRESHOLD if class_id == HOTDOG_CLASS_ID else HIGH_THRESHOLD
        if conf >= threshold:
            if class_id not in seen_classes or conf > seen_classes[class_id][1]:
                seen_classes[class_id] = (i, conf)

    indices = [idx for idx, _ in seen_classes.values()]
    detections = detections[indices]

    labels = [
        f"{CUSTOM_CLASSES[class_id]} {confidence:.2f}" if 0 <= class_id < len(CUSTOM_CLASSES)
        else f"Unknown {class_id} {confidence:.2f}"
        for class_id, confidence in zip(detections.class_id, detections.confidence)
    ]

    annotated = bbox_annotator.annotate(image_pil.copy(), detections)
    annotated = label_annotator.annotate(annotated, detections, labels)
    result = cv2.cvtColor(np.array(annotated), cv2.COLOR_RGB2BGR)

    cv2.imshow("RF-DETR Detection", result)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

camera.release()
cv2.destroyAllWindows()