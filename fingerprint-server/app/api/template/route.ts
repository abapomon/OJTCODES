import { readFile } from 'fs/promises';
import path from 'path';
import { NextRequest, NextResponse } from 'next/server';

export async function GET(req: NextRequest) {
  const id = req.nextUrl.searchParams.get('id');
  if (!id) {
    return NextResponse.json({ error: 'Missing ID' }, { status: 400 });
  }

  const filePath = path.join(process.cwd(), 'fingerprints', `${id}.bin`);
  try {
    const buf = await readFile(filePath); // Node.js Buffer

    if (buf.length !== 512) throw new Error('Invalid size');

    // ✅ Convert to Uint8Array directly — this is compatible with Web Response
    const uint8 = new Uint8Array(buf);

    return new NextResponse(uint8, {
      status: 200,
      headers: {
        'Content-Type': 'application/octet-stream',
        'Content-Length': '512'
      }
    });
  } catch (e) {
    return NextResponse.json({ error: 'Template not found' }, { status: 404 });
  }
}
