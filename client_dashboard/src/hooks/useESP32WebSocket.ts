import { useState, useEffect, useCallback, useRef } from 'react';
import { EcuData, ConnectionStatus, DataStreamEntry } from '@/types/ecuData';

interface ESP32WebSocketConfig {
  esp32_ip: string;
  port: number;
  reconnect_interval: number;
  max_reconnect_attempts: number;
}

interface UseESP32WebSocketReturn {
  ecuData: EcuData | null;
  connectionStatus: ConnectionStatus;
  dataStream: DataStreamEntry[];
  clearDataStream: () => void;
  connect: () => void;
  disconnect: () => void;
  sendCommand: (command: string) => void;
  updateConfig: (config: Partial<ESP32WebSocketConfig>) => void;
}

const DEFAULT_CONFIG: ESP32WebSocketConfig = {
  esp32_ip: '192.168.4.1', // Default ESP32 AP IP
  port: 80,
  reconnect_interval: 3000,
  max_reconnect_attempts: 10
};

export function useESP32WebSocket(): UseESP32WebSocketReturn {
  const [ecuData, setEcuData] = useState<EcuData | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>({
    connected: false,
    message: 'Disconnected'
  });
  const [dataStream, setDataStream] = useState<DataStreamEntry[]>([]);
  const [config, setConfig] = useState<ESP32WebSocketConfig>(DEFAULT_CONFIG);
  
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  const reconnectAttemptsRef = useRef(0);
  const shouldConnectRef = useRef(false);

  const addDataStreamEntry = useCallback((message: string, type: DataStreamEntry['type'] = 'info') => {
    const entry: DataStreamEntry = {
      timestamp: new Date(),
      message,
      type
    };
    
    setDataStream(prev => {
      const newStream = [entry, ...prev];
      return newStream.slice(0, 100); // Keep only last 100 entries
    });
  }, []);

  const connect = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      addDataStreamEntry('Already connected to ESP32', 'info');
      return;
    }

    shouldConnectRef.current = true;
    reconnectAttemptsRef.current = 0;

    const wsUrl = `ws://${config.esp32_ip}:${config.port}/ws`;
    addDataStreamEntry(`Connecting to ESP32 at ${wsUrl}...`, 'info');
    
    setConnectionStatus({
      connected: false,
      message: `Connecting to ${config.esp32_ip}...`
    });

    try {
      const ws = new WebSocket(wsUrl);
      wsRef.current = ws;

      ws.onopen = () => {
        reconnectAttemptsRef.current = 0;
        setConnectionStatus({
          connected: true,
          message: `Connected to ESP32 at ${config.esp32_ip}`
        });
        addDataStreamEntry('Connected to ESP32 WebSocket', 'success');
        
        // Send initial ping
        ws.send('ping');
      };

      ws.onmessage = (event) => {
        try {
          // Handle different message types
          if (event.data === 'pong') {
            // Pong response, connection is alive
            return;
          }

          const data = JSON.parse(event.data);
          
          // Check if it's ECU data
          if (data.engineRpm !== undefined || data.mapPressure !== undefined) {
            const ecuDataUpdate: EcuData = {
              mapPressure: data.mapPressure || 0,
              wastegatePosition: data.wastegatePosition || 0,
              tpsPosition: data.tpsPosition || 0,
              engineRpm: data.engineRpm || 0,
              targetBoost: data.targetBoost || 0,
              tcuProtectionActive: data.tcuProtectionActive || false,
              tcuLimpMode: data.tcuLimpMode || false,
              torqueRequest: data.torqueRequest || 0,
              timestamp: new Date(data.timestamp || Date.now())
            };
            
            setEcuData(ecuDataUpdate);
            
            // Add critical warnings to data stream
            if (ecuDataUpdate.tcuLimpMode) {
              addDataStreamEntry('TCU LIMP MODE ACTIVE!', 'error');
            } else if (ecuDataUpdate.tcuProtectionActive) {
              addDataStreamEntry('TCU Protection Active', 'warning');
            }
            
            if (ecuDataUpdate.engineRpm > 6000) {
              addDataStreamEntry(`High RPM: ${ecuDataUpdate.engineRpm.toFixed(0)}`, 'warning');
            }
          }
        } catch (error) {
          console.error('Error parsing WebSocket message:', error);
          addDataStreamEntry('Error parsing data from ESP32', 'error');
        }
      };

      ws.onclose = (event) => {
        setConnectionStatus({
          connected: false,
          message: 'Disconnected from ESP32'
        });
        
        if (shouldConnectRef.current && reconnectAttemptsRef.current < config.max_reconnect_attempts) {
          const reason = event.reason || 'Connection lost';
          addDataStreamEntry(`Connection lost: ${reason}. Reconnecting...`, 'warning');
          
          reconnectTimeoutRef.current = setTimeout(() => {
            reconnectAttemptsRef.current++;
            connect();
          }, config.reconnect_interval);
        } else if (reconnectAttemptsRef.current >= config.max_reconnect_attempts) {
          addDataStreamEntry('Max reconnection attempts reached', 'error');
          setConnectionStatus({
            connected: false,
            message: 'Failed to reconnect to ESP32'
          });
        } else {
          addDataStreamEntry('Disconnected from ESP32', 'info');
        }
      };

      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        addDataStreamEntry('WebSocket connection error', 'error');
        setConnectionStatus({
          connected: false,
          message: 'Connection error'
        });
      };

    } catch (error) {
      console.error('Failed to create WebSocket connection:', error);
      addDataStreamEntry('Failed to connect to ESP32', 'error');
      setConnectionStatus({
        connected: false,
        message: 'Connection failed'
      });
    }
  }, [config, addDataStreamEntry]);

  const disconnect = useCallback(() => {
    shouldConnectRef.current = false;
    
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
      reconnectTimeoutRef.current = null;
    }

    if (wsRef.current) {
      wsRef.current.close(1000, 'Manual disconnect');
      wsRef.current = null;
    }

    setConnectionStatus({
      connected: false,
      message: 'Disconnected'
    });
    addDataStreamEntry('Manually disconnected from ESP32', 'info');
  }, [addDataStreamEntry]);

  const sendCommand = useCallback((command: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(command);
      addDataStreamEntry(`Sent command: ${command}`, 'info');
    } else {
      addDataStreamEntry('Cannot send command: not connected', 'warning');
    }
  }, [addDataStreamEntry]);

  const clearDataStream = useCallback(() => {
    setDataStream([]);
    addDataStreamEntry('Data stream cleared', 'info');
  }, [addDataStreamEntry]);

  const updateConfig = useCallback((newConfig: Partial<ESP32WebSocketConfig>) => {
    setConfig(prev => ({ ...prev, ...newConfig }));
    addDataStreamEntry('ESP32 connection config updated', 'info');
  }, [addDataStreamEntry]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      disconnect();
    };
  }, [disconnect]);

  // Periodic ping to keep connection alive
  useEffect(() => {
    if (!connectionStatus.connected) return;

    const pingInterval = setInterval(() => {
      if (wsRef.current?.readyState === WebSocket.OPEN) {
        wsRef.current.send('ping');
      }
    }, 30000); // Ping every 30 seconds

    return () => clearInterval(pingInterval);
  }, [connectionStatus.connected]);

  return {
    ecuData,
    connectionStatus,
    dataStream,
    clearDataStream,
    connect,
    disconnect,
    sendCommand,
    updateConfig
  };
}
