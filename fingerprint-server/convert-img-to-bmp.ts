import { readFileSync, writeFileSync } from 'fs';
import path from 'path';

const WIDTH = 256;
const HEIGHT = 288;
const rawFile = path.join(__dirname, 'fingerprint-images/finger_<timestamp>.img'); // replace filename
const bmpFile = path.join(__dirname, 'fingerprint-images/finger_<timestamp>.bmp');

const raw = readFileSync(rawFile);
if (raw.length !== 36864) {
  console.error('Invalid raw size (expected 36864)');
  process.exit(1);
}

// Step 1: Expand 4-bit grayscale (2 pixels/byte) to 8-bit grayscale (1 pixel/byte)
const pixelData = Buffer.alloc(WIDTH * HEIGHT); // 73728 bytes
for (let i = 0; i < raw.length; i++) {
  const byte = raw[i];
  const high = (byte & 0xF0) >> 4;
  const low = byte & 0x0F;
  pixelData[i * 2] = high * 17;  // Scale 4-bit to 8-bit (0–255)
  pixelData[i * 2 + 1] = low * 17;
}

// Step 2: Create BMP header
function bmpHeader(width: number, height: number): Buffer {
  const headerSize = 54;
  const imageSize = width * height;
  const fileSize = headerSize + imageSize;

  const buffer = Buffer.alloc(headerSize);
  buffer.write('BM'); // Signature
  buffer.writeUInt32LE(fileSize, 2);
  buffer.writeUInt32LE(0, 6); // Reserved
  buffer.writeUInt32LE(headerSize, 10); // Offset to pixel data
  buffer.writeUInt32LE(40, 14); // DIB header size
  buffer.writeInt32LE(width, 18);
  buffer.writeInt32LE(-height, 22); // Negative for top-down
  buffer.writeUInt16LE(1, 26); // Color planes
  buffer.writeUInt16LE(8, 28); // Bits per pixel
  buffer.writeUInt32LE(0, 30); // Compression (none)
  buffer.writeUInt32LE(imageSize, 34);
  buffer.writeUInt32LE(0, 38); // X resolution
  buffer.writeUInt32LE(0, 42); // Y resolution
  buffer.writeUInt32LE(256, 46); // Color palette
  buffer.writeUInt32LE(0, 50); // Important colors

  return buffer;
}

// Step 3: Create grayscale palette (256 entries)
function grayscalePalette(): Buffer {
  const palette = Buffer.alloc(256 * 4); // 256 colors × 4 bytes (BGRA)
  for (let i = 0; i < 256; i++) {
    palette[i * 4 + 0] = i; // Blue
    palette[i * 4 + 1] = i; // Green
    palette[i * 4 + 2] = i; // Red
    palette[i * 4 + 3] = 0; // Reserved
  }
  return palette;
}

// Step 4: Combine all
const header = bmpHeader(WIDTH, HEIGHT);
const palette = grayscalePalette();
const bmp = Buffer.concat([header, palette, pixelData]);

writeFileSync(bmpFile, bmp);
console.log('✅ BMP saved:', bmpFile);
