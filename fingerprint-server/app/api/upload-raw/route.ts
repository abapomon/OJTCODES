import { NextRequest, NextResponse } from 'next/server';
import { writeFile } from 'fs/promises';
import path from 'path';
import { Buffer } from 'buffer';

const WIDTH = 256;
const HEIGHT = 288;
const DEPTH = 8;
const RAW_SIZE = (WIDTH * HEIGHT) / 2;

function createBMPHeader(): Buffer {
  const rowSize = ((DEPTH * WIDTH + 31) >> 5) << 2;
  const pixelDataSize = rowSize * HEIGHT;
  const paletteSize = 256 * 4;
  const headerSize = 54 + paletteSize;
  const fileSize = headerSize + pixelDataSize;

  const header = Buffer.alloc(headerSize);
  header.write('BM', 0);
  header.writeUInt32LE(fileSize, 2);
  header.writeUInt32LE(headerSize, 10); // pixel data offset
  header.writeUInt32LE(40, 14); // DIB header size
  header.writeInt32LE(WIDTH, 18);
  header.writeInt32LE(-HEIGHT, 22); // top-down bitmap
  header.writeUInt16LE(1, 26); // color planes
  header.writeUInt16LE(DEPTH, 28); // bpp
  header.writeUInt32LE(0, 30); // no compression
  header.writeUInt32LE(pixelDataSize, 34);
  header.writeUInt32LE(2835, 38); // resolution
  header.writeUInt32LE(2835, 42);
  header.writeUInt32LE(256, 46); // palette colors
  header.writeUInt32LE(0, 50);

  for (let i = 0; i < 256; i++) {
    const offset = 54 + i * 4;
    header[offset] = i;
    header[offset + 1] = i;
    header[offset + 2] = i;
    header[offset + 3] = 0;
  }

  return header;
}

export async function POST(req: NextRequest) {
  const buffer = Buffer.from(await req.arrayBuffer());

  if (buffer.length !== RAW_SIZE) {
    return NextResponse.json(
      { error: 'Invalid data size', received: buffer.length },
      { status: 400 }
    );
  }

  const pixelData = Buffer.alloc(WIDTH * HEIGHT);
  for (let i = 0; i < RAW_SIZE; i++) {
    const val = buffer[i];
    pixelData[i * 2] = val;
    pixelData[i * 2 + 1] = val;
  }

  const bmp = Buffer.concat([createBMPHeader(), pixelData]);
  const output = path.join(process.cwd(), 'public', 'fingerprint.bmp');
  await writeFile(output, bmp);

  return NextResponse.json({ success: true, path: '/fingerprint.bmp', size: bmp.length });
}
