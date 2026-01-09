import { useState, useEffect, useRef, useCallback } from 'react';
import { EcuData, ConnectionStatus, DataStreamEntry } from '@/types/ecuData';

interface UseWebSocketReturn {
  ecuData: EcuData | null;
  connectionStatus: ConnectionStatus;
  dataStream: DataStreamEntry[];
  sendMessage: (message: any) => void;
  clearDataStream: () => void;
  connect: () => void;
  disconnect: () => void;
  updateSimulatedData: (data: EcuData) => void;
}

export function useWebSocket(): UseWebSocketReturn {
  const [ecuData, setEcuData] = useState<EcuData | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>({
    connected: false,
    message: 'Disconnected'
  });
  const [dataStream, setDataStream] = useState<DataStreamEntry[]>([]);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<NodeJS.Timeout>();
  const reconnectAttempts = useRef(0);
  const maxReconnectAttempts = 5;

  const addToDataStream = useCallback((message: string, type: DataStreamEntry['type'] = 'info') => {
    const entry: DataStreamEntry = {
      timestamp: new Date(),
      message,
      type
    };
    
    setDataStream(prev => {
      const newStream = [...prev, entry].slice(-100); // Keep last 100 entries
      return newStream;
    });
  }, []);

  const connect = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      return;
    }

    try {
      const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
      const wsUrl = `${protocol}//${window.location.host}/ws`;
      
      setConnectionStatus({ connected: false, message: 'Connecting...' });
      addToDataStream('Attempting to connect to ECU...', 'info');
      
      const ws = new WebSocket(wsUrl);
      wsRef.current = ws;

      ws.onopen = () => {
        setConnectionStatus({ connected: true, message: 'Connected to ECU' });
        addToDataStream('Successfully connected to ECU', 'success');
        reconnectAttempts.current = 0;
      };

      ws.onmessage = (event) => {
        try {
          const message = JSON.parse(event.data);
          
          if (message.type === 'ecuData') {
            const data = message.data as EcuData;
            setEcuData(data);
            
            // Add formatted data to stream
            const streamMessage = `MAP:${data.mapPressure.toFixed(1)} | TGT:${data.targetBoost.toFixed(1)} | WG:${data.wastegatePosition.toFixed(1)}% | RPM:${data.engineRpm.toFixed(0)}`;
            addToDataStream(streamMessage, 'info');
            
            // Check for warnings
            if (data.tcuProtectionActive) {
              addToDataStream('Warning: TCU Protection Active', 'warning');
            }
            
            if (data.tcuLimpMode) {
              addToDataStream('Warning: TCU Limp Mode Active', 'warning');
            }
            
            if (Math.abs(data.mapPressure - data.targetBoost) > 10) {
              addToDataStream(`Warning: Target deviation ${(data.mapPressure - data.targetBoost).toFixed(1)} kPa`, 'warning');
            }
            
            if (Math.abs(data.mapPressure - data.targetBoost) <= 2) {
              addToDataStream('Target reached within tolerance', 'success');
            }
          } else if (message.type === 'error') {
            addToDataStream(`Error: ${message.message}`, 'error');
          }
        } catch (error) {
          console.error('Error parsing WebSocket message:', error);
          addToDataStream('Error parsing received data', 'error');
        }
      };

      ws.onclose = () => {
        setConnectionStatus({ connected: false, message: 'Connection lost' });
        addToDataStream('Connection to ECU lost', 'error');
        
        // Attempt to reconnect
        if (reconnectAttempts.current < maxReconnectAttempts) {
          reconnectAttempts.current++;
          const delay = Math.min(1000 * Math.pow(2, reconnectAttempts.current), 30000);
          
          addToDataStream(`Reconnecting in ${delay/1000}s... (attempt ${reconnectAttempts.current}/${maxReconnectAttempts})`, 'info');
          
          reconnectTimeoutRef.current = setTimeout(() => {
            connect();
          }, delay);
        } else {
          addToDataStream('Max reconnection attempts reached', 'error');
        }
      };

      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        addToDataStream('WebSocket connection error', 'error');
      };

    } catch (error) {
      console.error('Failed to create WebSocket connection:', error);
      setConnectionStatus({ connected: false, message: 'Connection failed' });
      addToDataStream('Failed to establish connection', 'error');
    }
  }, [addToDataStream]);

  const disconnect = useCallback(() => {
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
    }
    
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
    
    setConnectionStatus({ connected: false, message: 'Disconnected' });
    addToDataStream('Manually disconnected from ECU', 'info');
    reconnectAttempts.current = 0;
  }, [addToDataStream]);

  const sendMessage = useCallback((message: any) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(message));
    }
  }, []);

  const clearDataStream = useCallback(() => {
    setDataStream([]);
    addToDataStream('Data stream cleared', 'info');
  }, [addToDataStream]);

  const updateSimulatedData = useCallback((data: EcuData) => {
    setEcuData(data);
    
    // Add formatted data to stream
    const streamMessage = `MAP:${data.mapPressure.toFixed(1)} | TGT:${data.targetBoost.toFixed(1)} | WG:${data.wastegatePosition.toFixed(1)}% | RPM:${data.engineRpm.toFixed(0)}`;
    addToDataStream(streamMessage, 'info');
    
    // Check for warnings
    if (data.tcuProtectionActive) {
      addToDataStream('Warning: TCU Protection Active', 'warning');
    }
    
    if (data.tcuLimpMode) {
      addToDataStream('Warning: TCU Limp Mode Active', 'warning');
    }
    
    if (Math.abs(data.mapPressure - data.targetBoost) > 10) {
      addToDataStream(`Warning: Target deviation ${(data.mapPressure - data.targetBoost).toFixed(1)} kPa`, 'warning');
    }
    
    if (Math.abs(data.mapPressure - data.targetBoost) <= 2) {
      addToDataStream('Target reached within tolerance', 'success');
    }
  }, [addToDataStream]);

  useEffect(() => {
    // Auto-connect on mount
    connect();

    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connect]);

  return {
    ecuData,
    connectionStatus,
    dataStream,
    sendMessage,
    clearDataStream,
    connect,
    disconnect,
    updateSimulatedData
  };
}
