import socket
import struct

IMAGE_WIDTH = 256
IMAGE_HEIGHT = 288
IMAGE_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT  # 36864 bytes
UDP_PORT = 9999
OUTPUT_FILENAME = "fingerprint_from_udp.bmp"

def assemble_bmp_header(width, height):
    file_size = 54 + width * height
    return struct.pack(
        "<2sIHHIIIIHHIIIIII",
        b'BM',               # Signature
        file_size, 0, 0,     # File size, reserved1, reserved2
        54,                  # Offset to pixel data
        40,                  # Info header size
        width,
        height * -1,         # Negative for top-down bitmap
        1,                   # Planes
        8,                   # Bits per pixel
        0, file_size - 54,   # Compression (none), Image size
        2835, 2835,          # Pixels per meter (72 DPI)
        256, 0               # Colors used, important colors
    )

def generate_grayscale_palette():
    return b''.join(struct.pack('BBBB', i, i, i, 0) for i in range(256))

def main():
    print("🔌 Starting UDP server on port", UDP_PORT)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', UDP_PORT))

    image_data = bytearray()
    while len(image_data) < IMAGE_SIZE:
        packet, _ = sock.recvfrom(1024)  # You may increase if you're using 256-byte packets
        image_data += packet
        print(f"📦 Received {len(packet)} bytes, total: {len(image_data)}/{IMAGE_SIZE}")

    print("✅ All image data received")

    # Save BMP
    with open(OUTPUT_FILENAME, 'wb') as f:
        f.write(assemble_bmp_header(IMAGE_WIDTH, IMAGE_HEIGHT))
        f.write(generate_grayscale_palette())
        f.write(image_data)

    print(f"🖼️ Saved fingerprint as {OUTPUT_FILENAME}")

if __name__ == "__main__":
    main()
