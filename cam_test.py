import cv2

cam = cv2.VideoCapture(0)
cam.set(cv2.CAP_PROP_FPS, 60)

while True:
    ret, image = cam.read()
    image = cv2.flip(image,1)
    cv2.imshow('Camera Feed', image)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cam.release()
cv2.destroyAllWindows()
