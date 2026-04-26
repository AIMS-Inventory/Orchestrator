import aims_py
import json

def register_box_handler(payload_str):
    print("Received register_box event:")
    try:
        data = json.loads(payload_str)
        print(json.dumps(data, indent=2))
    except Exception as e:
        print("Failed to parse json:", e)

def get_active_pills_handler(payload_str):
    print("Received get_active_pills event:")
    try:
        data = json.loads(payload_str)
        print(json.dumps(data, indent=2))
    except Exception as e:
        print("Failed to parse json:", e)

aims_py.register_listener("register_box", register_box_handler)
aims_py.register_listener("get_active_pills", get_active_pills_handler)

print("Kiosk events python script loaded.")

