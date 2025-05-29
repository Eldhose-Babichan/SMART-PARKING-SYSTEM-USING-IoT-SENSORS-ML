from flask import Flask, request, jsonify
import os
import numpy as np
import cv2
import tensorflow as tf

app = Flask(__name__)

# Directory to save images
UPLOAD_FOLDER = 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# Load TFLite model and allocate tensors
def load_tflite_model(model_path):
    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()
    return interpreter

# Preprocess the input image
def preprocess_image(image_path, input_shape):
    img = cv2.imread(image_path)
    img = cv2.resize(img, (input_shape[1], input_shape[2]))  # Resize to model input size
    img = img / 255.0  # Normalize
    img = np.expand_dims(img, axis=0).astype(np.float32)  # Add batch dimension
    return img

# Run inference on the image
def classify_image(interpreter, image_path):
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
    
    # Get input shape
    input_shape = input_details[0]['shape']
    
    # Preprocess image
    image = preprocess_image(image_path, input_shape)
    
    # Set the input tensor
    interpreter.set_tensor(input_details[0]['index'], image)
    
    # Run inference
    interpreter.invoke()
    
    # Get the output tensor
    predictions = interpreter.get_tensor(output_details[0]['index'])
    
    # Assuming the model has 3 classes: ['No Vehicle', 'Bike', 'Car']
    class_names = ['Bike', 'Car', 'No Vehicle']
    predicted_class = class_names[np.argmax(predictions)]
    confidence = np.max(predictions) * 100
    
    return predicted_class, confidence

@app.route('/upload-image', methods=['POST'])
def upload_image():
    image = request.data
    if image:
        image_path = os.path.join(UPLOAD_FOLDER, 'captured_image.jpg')
        with open(image_path, 'wb') as f:
            f.write(image)
        # Load the model
        model_path = "vehicle_classification_model.tflite"  # Replace with your TFLite model path
        interpreter = load_tflite_model(model_path)
        # Classify the image
        predicted_class, confidence = classify_image(interpreter, image_path)
        return jsonify({'prediction': predicted_class, 'confidence': f'{confidence:.2f}%'})
    return '❌ No image received', 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=12345)