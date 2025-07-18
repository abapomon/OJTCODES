'use client';

import { useEffect, useState } from 'react';

type AttendanceLog = {
  id: number;
  fingerprint_id: number;
  timestamp: string;
};

export default function AttendanceLogsPage() {
  const [logs, setLogs] = useState<AttendanceLog[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetch('/api/logs')
      .then(res => res.json())
      .then(data => {
        setLogs(data);
        setLoading(false);
      })
      .catch(err => {
        console.error('Failed to fetch logs:', err);
        setLoading(false);
      });
  }, []);

  return (
    <main className="p-6 max-w-4xl mx-auto">
      <h1 className="text-2xl font-bold mb-4">📋 Attendance Logs</h1>
      {loading ? (
        <p>Loading logs...</p>
      ) : (
        <table className="w-full border-collapse border border-gray-300">
          <thead className="bg-gray-100">
            <tr>
              <th className="border px-4 py-2">#</th>
              <th className="border px-4 py-2">Fingerprint ID</th>
              <th className="border px-4 py-2">Timestamp</th>
            </tr>
          </thead>
          <tbody>
            {logs.map((log, idx) => (
              <tr key={log.id}>
                <td className="border px-4 py-2">{idx + 1}</td>
                <td className="border px-4 py-2">{log.fingerprint_id}</td>
                <td className="border px-4 py-2">{new Date(log.timestamp).toLocaleString()}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}    
    </main>
  );
}
