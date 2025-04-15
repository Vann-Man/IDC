import cv2
import uuid

cam = cv2.VideoCapture(0)
cam.set(cv2.CAP_PROP_FPS, 60)

while True:
    ret, image = cam.read()
    cv2.imshow('Camera Feed', image)
    if cv2.waitKey(1) & 0xFF == ord('t'):
        cv2.imwrite(f"photos_burg/{uuid.uuid4()}.jpg", image)
        print("Photo taken")
    elif cv2.waitKey(1) & 0xFF == ord('q'):
        break

cam.release()
cv2.destroyAllWindows()
