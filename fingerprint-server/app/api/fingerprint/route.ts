import { NextRequest, NextResponse } from 'next/server';
import { writeFile, mkdir, readFile } from 'fs/promises';
import path from 'path';
import { matchWithReference } from '@/lib/matcher';

const UPLOAD_DIR = path.join(process.cwd(), 'fingerprints');

// POST handler for ESP32 fingerprint upload
export async function POST(req: NextRequest) {
  const buffer = await req.arrayBuffer();
  const fingerprintData = Buffer.from(buffer);

  await mkdir(UPLOAD_DIR, { recursive: true });

  const mode = req.nextUrl.searchParams.get('mode');
  const timestamp = Date.now();
  const fileName = mode === 'register' ? 'reference.bin' : `scan_${timestamp}.bin`;
  const filePath = path.join(UPLOAD_DIR, fileName);

  await writeFile(filePath, fingerprintData);
  console.log(`Saved fingerprint to ${fileName}`);

  if (mode === 'register') {
    return NextResponse.json({ status: 'registered', file: fileName });
  }

  // Compare with stored reference
  const referencePath = path.join(UPLOAD_DIR, 'reference.bin');
  let score = 0;
  let matched = false;

  try {
    const referenceData = await readFile(referencePath);
    ({ score, matched } = await matchWithReference(fingerprintData, referenceData));
  } catch (err) {
    return NextResponse.json({ error: 'Reference not found', status: 'error' }, { status: 404 });
  }

  return NextResponse.json({
    status: 'scanned',
    file: fileName,
    score,
    matched,
  });
}
