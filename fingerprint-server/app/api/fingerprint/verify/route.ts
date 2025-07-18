import { NextRequest, NextResponse } from 'next/server';
import { readdir, readFile } from 'fs/promises';
import path from 'path';

function countBits(byte: number): number {
  // Count number of '1' bits in byte
  let count = 0;
  while (byte) {
    count += byte & 1;
    byte >>= 1;
  }
  return count;
}

function compareTemplates(a: Buffer, b: Buffer): number {
  let totalBits = 512 * 8;
  let matchingBits = 0;

  for (let i = 0; i < 512; i++) {
    const xor = a[i] ^ b[i];
    const diffBits = countBits(xor);
    matchingBits += 8 - diffBits; // Count matching bits
  }

  return (matchingBits / totalBits) * 100;
}


export async function POST(req: NextRequest) {
  const data = await req.arrayBuffer();

  if (data.byteLength !== 512) {
    return NextResponse.json({ error: 'Invalid fingerprint size' }, { status: 400 });
  }

  const incoming = Buffer.from(data);
  const dir = path.join(process.cwd(), 'fingerprints');

  try {
    const files = await readdir(dir);
    let bestScore = 0;
    let bestMatchId: string | null = null;

    for (const file of files) {
      const stored = await readFile(path.join(dir, file));
      const score = compareTemplates(incoming, stored);

      if (score > bestScore) {
        bestScore = score;
        bestMatchId = file.replace('.bin', '');
      }
    }

    return NextResponse.json({
      match: bestScore >= 85,
      id: bestMatchId,
      confidence: bestScore.toFixed(2),
    });
  } catch (err) {
    return NextResponse.json({ error: 'Server error', details: err }, { status: 500 });
  }
}
