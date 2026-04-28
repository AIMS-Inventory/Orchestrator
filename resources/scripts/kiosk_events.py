import aims_py
import json

def register_box_handler(payload_str):
    print("Received register_box event:")
    try:
        data = json.loads(payload_str)
        print(json.dumps(data, indent=2))

        box_id = data.get("box_id")
        shelf_code = data.get("shelf_code")
        position = data.get("position")
        placed_by = data.get("placed_by", "Unknown")
        pills = data.get("pills", [])

        if box_id is None or shelf_code is None or position is None:
            print("Error: Missing required fields (box_id, shelf_code, position)")
            return

        box_id_str = str(box_id)

        if isinstance(position, list):
            position_str = f"{position[0]},{position[1]}"
        else:
            position_str = str(position)

        box = aims_py.Box()
        box.id = box_id_str
        box.contents = aims_py.BoxContents()
        box.contents.placed_by = placed_by
        box.contents.pills = pills

        success = aims_py.register_box(shelf_code, position_str, box)
        if success:
            print(f"Successfully registered box '{box_id_str}' at shelf {shelf_code}, position {position_str}")
        else:
            print(f"Failed to register box '{box_id_str}' - shelf code {shelf_code} not found")

    except Exception as e:
        print(f"Error processing register_box event: {e}")

def get_active_pills_handler(payload_str):
    print("Received get_active_pills event:")
    try:
        data = json.loads(payload_str)
        print(json.dumps(data, indent=2))

        shelves = aims_py.get_shelves()

        active_pills = {}
        for shelf in shelves:
            for position, box in shelf.boxes.items():
                for pill in box.contents.pills:
                    if pill not in active_pills:
                        active_pills[pill] = {
                            "count": 0,
                            "locations": []
                        }
                    active_pills[pill]["count"] += 1
                    active_pills[pill]["locations"].append({
                        "shelf": shelf.name,
                        "shelf_code": shelf.code,
                        "position": position,
                        "box_id": box.id,
                        "placed_by": box.contents.placed_by
                    })

        print("Active pills on shelves:")
        print(json.dumps(active_pills, indent=2))

    except Exception as e:
        print(f"Error retrieving active pills: {e}")

aims_py.register_listener("register_box", register_box_handler)
aims_py.register_listener("get_active_pills", get_active_pills_handler)

print("Kiosk events python script loaded.")
