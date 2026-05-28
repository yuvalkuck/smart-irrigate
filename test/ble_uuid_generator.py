import uuid
# Generates a random UUID and prints it as a C-style hex array
u = uuid.uuid4()
print("ble_uuid128_t var = BLE_UUID128_INIT(" + ", ".join([f"0x{b:02x}" for b in reversed(u.bytes)]) + ")")
