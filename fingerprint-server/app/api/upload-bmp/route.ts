import { NextRequest } from "next/server";
import fs from "fs";
import path from "path";

export async function POST(req: NextRequest) {
  const raw = await req.arrayBuffer();
  const buffer = Buffer.from(raw);

  const filePath = path.join(process.cwd(), "public", "fingerprint.bmp");
  fs.writeFileSync(filePath, buffer);

  console.log(`✅ Image saved at /public/fingerprint.bmp`);
  return new Response("✅ Image saved", { status: 200 });
}
