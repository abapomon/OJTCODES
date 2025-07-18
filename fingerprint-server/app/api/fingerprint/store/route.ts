import { writeFile, mkdir } from 'fs/promises';
import { existsSync } from 'fs';
import path from 'path';
import { NextRequest, NextResponse } from 'next/server';

export async function POST(req: NextRequest) {
  const id = req.nextUrl.searchParams.get('id');
  if (!id) {
    return NextResponse.json({ error: 'Missing ID' }, { status: 400 });
  }

  const buffer = await req.arrayBuffer();
  if (buffer.byteLength !== 512) {
    return NextResponse.json({ error: 'Invalid template size' }, { status: 400 });
  }

  const dirPath = path.join(process.cwd(), 'fingerprints');
  if (!existsSync(dirPath)) {
    await mkdir(dirPath, { recursive: true });
  }

  const filePath = path.join(dirPath, `${id}.tpl`);
  await writeFile(filePath, Buffer.from(buffer));

  console.log(`✅ Stored fingerprint template for ID ${id}`);
  return NextResponse.json({ message: `Template stored for ID ${id}` });
}
