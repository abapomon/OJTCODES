import { NextRequest, NextResponse } from 'next/server';
import { writeFile } from 'fs/promises';
import path from 'path';
import { mkdirSync, existsSync } from 'fs';

export async function POST(req: NextRequest) {
  try {
    const buffer = Buffer.from(await req.arrayBuffer());

    if (buffer.length !== 36864) {
      return NextResponse.json({ error: 'Invalid image size' }, { status: 400 });
    }

    // Make sure directory exists
    const dir = path.join(process.cwd(), 'fingerprint-images');
    if (!existsSync(dir)) mkdirSync(dir);

    // Save as raw binary or .img
    const timestamp = Date.now();
    const filepath = path.join(dir, `finger_${timestamp}.img`);
    await writeFile(filepath, buffer);

    return NextResponse.json({ success: true, message: '🖼️ Image saved', filename: `finger_${timestamp}.img` });
  } catch (err) {
    console.error(err);
    return NextResponse.json({ error: 'Server error' }, { status: 500 });
  }
}
