import { NextRequest, NextResponse } from 'next/server';
import mysql from 'mysql2/promise';

export async function GET(req: NextRequest) {
  try {
    const conn = await mysql.createConnection({
      host: process.env.DB_HOST,
      user: process.env.DB_USER,
      password: process.env.DB_PASSWORD,
      database: process.env.DB_NAME,
    });

    const [rows] = await conn.execute(
      'SELECT id, fingerprint_id, timestamp FROM attendance ORDER BY timestamp DESC LIMIT 100'
    );
    await conn.end();

    return NextResponse.json(rows);
  } catch (err) {
    console.error('❌ Fetch logs error:', err);
    return NextResponse.json({ success: false, error: String(err) }, { status: 500 });
  }
}
