// Handles POST /api/enroll?id=123

import { NextRequest, NextResponse } from 'next/server';
import { writeFile } from 'fs/promises';
import path from 'path';

export async function POST(req: NextRequest) {
  const id = req.nextUrl.searchParams.get('id');

  if (!id) return NextResponse.json({ error: 'Missing ID' }, { status: 400 });

  const data = await req.arrayBuffer();
  if (data.byteLength !== 512)
    return NextResponse.json({ error: 'Invalid fingerprint size' }, { status: 400 });

  const dir = path.join(process.cwd(), 'fingerprints');
  await writeFile(path.join(dir, `${id}.bin`), Buffer.from(data));

  return NextResponse.json({ message: `Template stored for ID ${id}` });
}
