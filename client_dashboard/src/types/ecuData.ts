export interface EcuData {
  mapPressure: number;
  wastegatePosition: number;
  tpsPosition: number;
  engineRpm: number;
  targetBoost: number;
  tcuProtectionActive: boolean;
  tcuLimpMode: boolean;
  torqueRequest?: number;
  timestamp?: Date;
}

export interface SystemSettings {
  maxBoostLimit: number;
  maxRpmLimit: number;
  audioAlertsEnabled: boolean;
  ecuAddress: string;
}

export interface ConnectionStatus {
  connected: boolean;
  message?: string;
}

export interface DataStreamEntry {
  timestamp: Date;
  message: string;
  type: 'info' | 'warning' | 'success' | 'error';
}

export interface GaugeConfig {
  id: string;
  title: string;
  subtitle: string;
  unit: string;
  min: number;
  max: number;
  value: number;
  target?: number;
  color: string;
  warningThreshold?: number;
  dangerThreshold?: number;
}
