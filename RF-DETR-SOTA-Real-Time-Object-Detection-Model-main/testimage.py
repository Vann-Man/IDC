from rfdetr import RFDETRBase
import supervision as sv
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt

# Define your custom classes (34 Mahjong tile classes)
CUSTOM_CLASSES = [
    "SANDWICH", "WEINER", "BURGER"
]

# Load the image
image = Image.open("testBURGER.png.png").convert("RGB")

# Initialize the RF-DETR model with pre-trained weights
model = RFDETRBase(pretrain_weights="checkpoint_best_ema.pth")

# Run inference on the image with a confidence threshold of 0.5
detections = model.predict(image, threshold=0.5)

# Define a color palette for visualization
color = sv.ColorPalette.from_hex([
    "#ffff00", "#ff9b00", "#ff8080", "#ff66b2", "#ff66ff", "#b266ff",
    "#9999ff", "#3399ff", "#66ffff", "#33ff99", "#66ff66", "#99ff00"
])

# Calculate optimal text scale and line thickness based on image resolution
text_scale = sv.calculate_optimal_text_scale(resolution_wh=image.size)
thickness = sv.calculate_optimal_line_thickness(resolution_wh=image.size)

# Set up annotators for bounding boxes and labels
bbox_annotator = sv.BoxAnnotator(color=color, thickness=thickness)
label_annotator = sv.LabelAnnotator(
    color=color,
    text_color=sv.Color.BLACK,
    text_scale=text_scale,
    smart_position=True
)

# Create labels for each detection using custom class names and confidence scores
labels = [
    f"{CUSTOM_CLASSES[class_id]} {confidence:.2f}"
    for class_id, confidence
    in zip(detections.class_id, detections.confidence)
]

# Annotate the image with bounding boxes and labels
annotated_image = image.copy()
annotated_image = bbox_annotator.annotate(annotated_image, detections)
annotated_image = label_annotator.annotate(annotated_image, detections, labels)

# Convert the PIL Image to a NumPy array for matplotlib
annotated_image_np = np.array(annotated_image)

# Display the annotated image using matplotlib
plt.figure(figsize=(10, 10))  # Set the figure size (adjust as needed)
plt.imshow(annotated_image_np)
plt.axis('off')  # Hide axes for a cleaner display
plt.title("RF-DETR Mahjong Tile Detection Results")  # Updated title
plt.show()

# Optionally, save the annotated image
#annotated_image.save("annotated_demo.jpg")