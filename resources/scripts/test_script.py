import aims_py
def event_handler():
    print("Event handler called")
    
aims_py.register_listener("test_event", event_handler)
print("hi im a test script")