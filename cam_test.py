import cv2
import uuid
import time

cam = cv2.VideoCapture(0)
cam.set(cv2.CAP_PROP_FPS, 60)

last_capture_time = time.time()  # Track the last capture time

while True:
    ret, image = cam.read()
    cv2.imshow('Camera Feed', image)

    current_time = time.time()
    # Take a picture every 1 second
    if current_time - last_capture_time >= 3:
        cv2.imwrite(f"/Users/kevinorjalo/Desktop/food_pic/{uuid.uuid4()}.jpg", image)
        print("Photo taken", uuid.uuid4())
        last_capture_time = current_time  # Update the last capture time

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cam.release()
cv2.destroyAllWindows()