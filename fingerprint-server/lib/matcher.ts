export async function matchWithReference(
  uploaded: Buffer,
  reference: Buffer
): Promise<{ score: number; matched: boolean }> {
  // 🔁 Replace this with real matching logic later (DLL, WASM, or Python)
  const score = Math.floor(Math.random() * 100);
  const matched = score >= 50;

  return { score, matched };
}
