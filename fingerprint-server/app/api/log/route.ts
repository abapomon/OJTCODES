import { NextRequest, NextResponse } from 'next/server';
import mysql from 'mysql2/promise';

export async function POST(req: NextRequest) {
  try {
    const { id, timestamp } = await req.json();

    const conn = await mysql.createConnection({
      host: process.env.DB_HOST,
      user: process.env.DB_USER,
      password: process.env.DB_PASSWORD,
      database: process.env.DB_NAME,
    });

    await conn.execute(
      'INSERT INTO attendance (fingerprint_id, timestamp) VALUES (?, ?)',
      [id, timestamp]
    );
    await conn.end();

    return NextResponse.json({ success: true });
  } catch (err) {
    console.error('❌ DB Insert Error:', err);
    return NextResponse.json({ success: false, error: String(err) }, { status: 500 });
  }
}
