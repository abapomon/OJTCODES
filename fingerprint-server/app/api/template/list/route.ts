// /api/template/list/route.ts
import { readdir } from 'fs/promises';
import path from 'path';
import { NextResponse } from 'next/server';

export async function GET() {
  try {
    const dir = path.join(process.cwd(), 'fingerprints');
    const files = await readdir(dir);
    const ids = files
      .filter(f => f.endsWith('.bin'))
      .map(f => f.replace('.bin', ''));
    return NextResponse.json(ids);
  } catch (err) {
    return NextResponse.json({ error: 'Server error' }, { status: 500 });
  }
}
