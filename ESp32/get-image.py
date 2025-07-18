import serial, time, argparse, struct

IMAGE_WIDTH = 256
IMAGE_HEIGHT = 288
IMAGE_DEPTH = 8
IMAGE_START_SIGNATURE = b'\xAA'

def assembleBMPHeader(width, height, depth, includePalette=False):
    bmpHeader = struct.Struct("<2s3L LLl2H6L")
    bmpPaletteEntry = struct.Struct("4B")
    byteWidth = ((depth * width + 31) // 32) * 4
    numColours = 2**depth
    bmpPaletteSize = bmpPaletteEntry.size * numColours
    imageSize = byteWidth * height
    if includePalette:
        fileSize = bmpHeader.size + bmpPaletteSize + imageSize
        rasterOffset = bmpHeader.size + bmpPaletteSize
    else:
        fileSize = bmpHeader.size + imageSize
        rasterOffset = bmpHeader.size

    BMP_INFOHEADER_SZ = 40
    DPI = 2835  # 72 DPI

    bmpHeaderBytes = bmpHeader.pack(
        b"BM", fileSize, 0, rasterOffset,
        BMP_INFOHEADER_SZ, width, -height, 1, depth,
        0, imageSize, DPI, DPI, 0, 0
    )

    if includePalette:
        palette = [bmpPaletteEntry.pack(i, i, i, 0) for i in range(numColours)]
        return bmpHeaderBytes + b''.join(palette)

    return bmpHeaderBytes

def getFingerprintImage(portNum, baudRate, outputFileName):
    try:
        port = serial.Serial(portNum, baudRate, timeout=0.1, inter_byte_timeout=0.1)
    except Exception as e:
        print('❌ Port open failed:', e)
        return False

    with open(outputFileName, "wb") as outFile:
        outFile.write(assembleBMPHeader(IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_DEPTH, True))
        time.sleep(1)  # wait for ESP32 to reboot

        currByte = b''
        while currByte != IMAGE_START_SIGNATURE:
            currByte = port.read()
            print(currByte.decode(errors='ignore'), end='')

        totalBytesExpected = (IMAGE_WIDTH * IMAGE_HEIGHT) // 2
        for i in range(totalBytesExpected):
            b = port.read()
            if not b:
                print("❌ Read timed out.")
                return False
            outFile.write(b * 2)  # duplicate byte to match pixel format

        while port.in_waiting:
            port.read()  # flush leftover

    print(f"✅ Image saved as {outputFileName}")
    port.close()
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Save fingerprint image from ESP32 via FPM")
    parser.add_argument("portNum", help="e.g., COM12")
    parser.add_argument("baudRate", type=int, help="e.g., 57600")
    parser.add_argument("outputFileName", help="e.g., fingerprint.bmp")
    args = parser.parse_args()

    getFingerprintImage(args.portNum, args.baudRate, args.outputFileName)
