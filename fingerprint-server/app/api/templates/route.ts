// /app/api/fingerprint/templates/route.ts
import { readdir, readFile } from 'fs/promises';
import path from 'path';
import { NextResponse } from 'next/server';

export async function GET() {
  const dir = path.join(process.cwd(), 'fingerprints');

  try {
    const files = await readdir(dir);
    const templates = [];

    for (const file of files) {
      if (file.endsWith('.tpl')) {
        const id = file.replace('.tpl', '');
        const data = await readFile(path.join(dir, file));
        if (data.length === 512) {
          templates.push({
            id,
            template: data.toString('base64'),
          });
        }
      }
    }

    return NextResponse.json({ templates });
  } catch (err) {
    return NextResponse.json({ error: 'Failed to read templates' }, { status: 500 });
  }
}
