import { useEffect, useRef } from 'react';
import { Trash2 } from 'lucide-react';
import { DataStreamEntry } from '@/types/ecuData';

interface DataStreamProps {
  entries: DataStreamEntry[];
  onClear: () => void;
}

export function DataStream({ entries, onClear }: DataStreamProps) {
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [entries]);

  const getEntryClass = (type: DataStreamEntry['type']) => {
    switch (type) {
      case 'success':
        return 'text-automotive-success';
      case 'warning':
        return 'text-automotive-warning';
      case 'error':
        return 'text-automotive-danger';
      default:
        return 'text-gray-300';
    }
  };

  const formatTimestamp = (timestamp: Date) => {
    return timestamp.toLocaleTimeString('en-US', {
      hour12: false,
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    }) + '.' + timestamp.getMilliseconds().toString().padStart(3, '0');
  };

  return (
    <div className="bg-automotive-card rounded-xl p-6 border border-gray-700">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-orbitron font-bold text-white">LIVE DATA STREAM</h3>
        <button 
          onClick={onClear}
          className="text-automotive-accent hover:text-blue-400 transition-colors"
        >
          <Trash2 className="w-4 h-4 inline mr-1" />
          Clear
        </button>
      </div>
      
      <div 
        ref={scrollRef}
        className="data-stream font-mono text-sm space-y-1"
      >
        {entries.length === 0 ? (
          <div className="text-gray-400">No data received yet...</div>
        ) : (
          entries.map((entry, index) => (
            <div key={index} className={getEntryClass(entry.type)}>
              {formatTimestamp(entry.timestamp)} | {entry.message}
            </div>
          ))
        )}
      </div>
    </div>
  );
}
