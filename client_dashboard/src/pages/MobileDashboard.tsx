import { useState, useEffect, useRef } from 'react';
import { Card } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { Alert, AlertDescription } from '@/components/ui/alert';
import { Wifi, WifiOff, Bluetooth, BluetoothOff, AlertCircle } from 'lucide-react';
import { Progress } from '@/components/ui/progress';
import { cn } from '@/lib/utils';

interface CANData {
  map_pressure: number;      // 100-250 kPa
  wastegate_pos: number;     // 0-100 %
  tps_position: number;      // 0-100 %
  engine_rpm: number;        // 0-7000 RPM
  target_boost: number;      // 100-250 kPa
  tcu_status: number;        // 0=OK, 1=WARN, 2=ERROR
  timestamp: number;
}

interface GaugeProps {
  value: number;
  min: number;
  max: number;
  label: string;
  unit: string;
  color: string;
  warningThreshold?: number;
  dangerThreshold?: number;
}

const CircularGauge = ({ value, min, max, label, unit, color, warningThreshold, dangerThreshold }: GaugeProps) => {
  const percentage = ((value - min) / (max - min)) * 100;
  const radius = 60;
  const strokeWidth = 8;
  const normalizedRadius = radius - strokeWidth / 2;
  const circumference = normalizedRadius * 2 * Math.PI;
  const strokeDashoffset = circumference - (percentage / 100) * circumference * 0.75;
  
  let gaugeColor = color;
  if (dangerThreshold && value >= dangerThreshold) {
    gaugeColor = '#FF4444';
  } else if (warningThreshold && value >= warningThreshold) {
    gaugeColor = '#FFA500';
  }
  
  return (
    <Card className="p-4 bg-gray-900 border-gray-700">
      <div className="flex flex-col items-center">
        <svg
          height={radius * 2}
          width={radius * 2}
          className="transform -rotate-135"
        >
          {/* Background arc */}
          <circle
            stroke="#2d2d2d"
            fill="transparent"
            strokeWidth={strokeWidth}
            strokeDasharray={`${circumference * 0.75} ${circumference}`}
            r={normalizedRadius}
            cx={radius}
            cy={radius}
          />
          {/* Value arc */}
          <circle
            stroke={gaugeColor}
            fill="transparent"
            strokeWidth={strokeWidth}
            strokeDasharray={`${circumference * 0.75} ${circumference}`}
            strokeDashoffset={strokeDashoffset}
            r={normalizedRadius}
            cx={radius}
            cy={radius}
            className="transition-all duration-300"
          />
        </svg>
        <div className="text-center -mt-20">
          <div className="text-2xl font-bold text-white">{value.toFixed(0)}</div>
          <div className="text-xs text-gray-400">{unit}</div>
        </div>
        <div className="text-sm font-medium text-gray-300 mt-4">{label}</div>
      </div>
    </Card>
  );
};

export default function MobileDashboard() {
  const [canData, setCanData] = useState<CANData>({
    map_pressure: 100,
    wastegate_pos: 0,
    tps_position: 0,
    engine_rpm: 0,
    target_boost: 100,
    tcu_status: 0,
    timestamp: Date.now()
  });
  
  const [connectionStatus, setConnectionStatus] = useState<'disconnected' | 'connecting' | 'connected'>('disconnected');
  const [connectionType, setConnectionType] = useState<'wifi' | 'bluetooth'>('wifi');
  const [esp32Address, setEsp32Address] = useState('192.168.4.1'); // Default ESP32 AP address
  const wsRef = useRef<WebSocket | null>(null);
  
  // WebSocket connection
  const connectWiFi = () => {
    setConnectionStatus('connecting');
    
    try {
      const ws = new WebSocket(`ws://${esp32Address}:81/ws`);
      
      ws.onopen = () => {
        setConnectionStatus('connected');
        console.log('Connected to ESP32 via WebSocket');
      };
      
      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          setCanData({
            ...data,
            timestamp: Date.now()
          });
        } catch (e) {
          console.error('Failed to parse CAN data:', e);
        }
      };
      
      ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        setConnectionStatus('disconnected');
      };
      
      ws.onclose = () => {
        setConnectionStatus('disconnected');
      };
      
      wsRef.current = ws;
    } catch (error) {
      console.error('Failed to connect:', error);
      setConnectionStatus('disconnected');
    }
  };
  
  const disconnect = () => {
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
    setConnectionStatus('disconnected');
  };
  
  // Simulate data for demo (remove when connected to real ESP32)
  useEffect(() => {
    if (connectionStatus !== 'connected') {
      const interval = setInterval(() => {
        setCanData(prev => ({
          map_pressure: 100 + Math.random() * 150,
          wastegate_pos: Math.random() * 100,
          tps_position: Math.random() * 100,
          engine_rpm: Math.random() * 7000,
          target_boost: 100 + Math.random() * 150,
          tcu_status: Math.floor(Math.random() * 3),
          timestamp: Date.now()
        }));
      }, 1000);
      
      return () => clearInterval(interval);
    }
  }, [connectionStatus]);
  
  const getTcuStatusColor = () => {
    switch (canData.tcu_status) {
      case 0: return 'bg-green-500';
      case 1: return 'bg-yellow-500';
      case 2: return 'bg-red-500 animate-pulse';
      default: return 'bg-gray-500';
    }
  };
  
  const getTcuStatusText = () => {
    switch (canData.tcu_status) {
      case 0: return 'TCU OK';
      case 1: return 'TCU WARN';
      case 2: return 'TCU ERROR';
      default: return 'UNKNOWN';
    }
  };
  
  return (
    <div className="min-h-screen bg-gray-950 p-4">
      {/* Header */}
      <div className="mb-4">
        <h1 className="text-2xl font-bold text-white mb-2">ECU Dashboard</h1>
        
        {/* Connection Status */}
        <div className="flex items-center justify-between mb-4">
          <div className="flex items-center space-x-2">
            {connectionType === 'wifi' ? (
              connectionStatus === 'connected' ? <Wifi className="text-green-500" /> : <WifiOff className="text-gray-500" />
            ) : (
              connectionStatus === 'connected' ? <Bluetooth className="text-green-500" /> : <BluetoothOff className="text-gray-500" />
            )}
            <Badge variant={connectionStatus === 'connected' ? 'default' : 'secondary'}>
              {connectionStatus === 'connecting' ? 'Connecting...' : connectionStatus}
            </Badge>
          </div>
          
          {connectionStatus === 'disconnected' ? (
            <Button onClick={connectWiFi} size="sm">
              Connect WiFi
            </Button>
          ) : (
            <Button onClick={disconnect} variant="destructive" size="sm">
              Disconnect
            </Button>
          )}
        </div>
        
        {/* Connection Info */}
        {connectionStatus === 'disconnected' && (
          <Alert className="mb-4">
            <AlertCircle className="h-4 w-4" />
            <AlertDescription>
              Connect to ESP32 AP (ECU_Dashboard) or enter ESP32 IP address
            </AlertDescription>
          </Alert>
        )}
      </div>
      
      {/* Gauges Grid */}
      <div className="grid grid-cols-2 gap-4 mb-4">
        <CircularGauge
          value={canData.map_pressure}
          min={100}
          max={250}
          label="MAP Pressure"
          unit="kPa"
          color="#00D4FF"
          warningThreshold={200}
          dangerThreshold={230}
        />
        
        <CircularGauge
          value={canData.wastegate_pos}
          min={0}
          max={100}
          label="Wastegate"
          unit="%"
          color="#00FF88"
        />
        
        <CircularGauge
          value={canData.tps_position}
          min={0}
          max={100}
          label="TPS"
          unit="%"
          color="#FFD700"
        />
        
        <CircularGauge
          value={canData.engine_rpm}
          min={0}
          max={7000}
          label="Engine"
          unit="RPM"
          color="#FF6B35"
          warningThreshold={5500}
          dangerThreshold={6500}
        />
      </div>
      
      {/* Target Boost and TCU Status */}
      <div className="grid grid-cols-2 gap-4">
        <CircularGauge
          value={canData.target_boost}
          min={100}
          max={250}
          label="Target Boost"
          unit="kPa"
          color="#FFD700"
        />
        
        <Card className="p-4 bg-gray-900 border-gray-700 flex flex-col items-center justify-center">
          <div className={cn(
            "w-16 h-16 rounded-full mb-2",
            getTcuStatusColor()
          )} />
          <div className="text-lg font-medium text-white">{getTcuStatusText()}</div>
          <div className="text-xs text-gray-400 mt-1">Transmission Control</div>
        </Card>
      </div>
      
      {/* Data Age Indicator */}
      <div className="mt-4 text-center text-xs text-gray-500">
        Last update: {new Date(canData.timestamp).toLocaleTimeString()}
      </div>
    </div>
  );
}