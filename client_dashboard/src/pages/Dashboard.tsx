import { useMemo, useState } from 'react';
import { Gauge as GaugeIcon, Shield, Settings, Circle, Layout } from 'lucide-react';
import { Gauge } from '@/components/Gauge';
import { StatusBanner } from '@/components/StatusBanner';
import { DataStream } from '@/components/DataStream';
import { SystemControls } from '@/components/SystemControls';
import { GaugeSettings, GaugeDisplaySettings } from '@/components/GaugeSettings';
import { GaugeDataSimulator } from '@/components/GaugeDataSimulator';
import { Button } from '@/components/ui/button';
import { useWebSocket } from '@/hooks/useWebSocket';
import { GaugeConfig } from '@/types/ecuData';

export default function Dashboard() {
  const {
    ecuData,
    connectionStatus,
    dataStream,
    clearDataStream,
    connect,
    disconnect,
    updateSimulatedData
  } = useWebSocket();

  const [displaySettings, setDisplaySettings] = useState<GaugeDisplaySettings>({
    size: 'medium',
    columns: 3,
    showTitles: true,
    showValues: true,
    showTargets: true,
    compactMode: false,
    gaugeStyle: 'circular',
    visibleGauges: {
      map: true,
      wastegate: true,
      tps: true,
      rpm: true,
      target: true,
      tcuStatus: true
    },
    customPositions: false,
    positions: {}
  });

  const gaugeConfigs = useMemo((): GaugeConfig[] => {
    if (!ecuData) {
      return [
        { id: 'map', title: 'BOOST PRESSURE', subtitle: 'MAP Sensor (kPa)', unit: 'kPa', min: 100, max: 250, value: 0, target: 0, color: '#00D4FF' },
        { id: 'wastegate', title: 'WASTEGATE', subtitle: 'Position (%)', unit: '%', min: 0, max: 100, value: 0, color: '#00FF88' },
        { id: 'tps', title: 'THROTTLE', subtitle: 'TPS (%)', unit: '%', min: 0, max: 100, value: 0, color: '#FFD700' },
        { id: 'rpm', title: 'ENGINE RPM', subtitle: 'Revolutions/min', unit: 'RPM', min: 0, max: 7000, value: 0, color: '#FF6B35', warningThreshold: 6000, dangerThreshold: 6500 },
        { id: 'target', title: 'TARGET BOOST', subtitle: 'Calculated (kPa)', unit: 'kPa', min: 100, max: 250, value: 0, color: '#FFD700' },
      ];
    }

    return [
      {
        id: 'map',
        title: 'BOOST PRESSURE',
        subtitle: 'MAP Sensor (kPa)',
        unit: 'kPa',
        min: 100,
        max: 250,
        value: ecuData.mapPressure,
        target: ecuData.targetBoost,
        color: '#00D4FF',
        warningThreshold: 230,
        dangerThreshold: 245
      },
      {
        id: 'wastegate',
        title: 'WASTEGATE',
        subtitle: 'Position (%)',
        unit: '%',
        min: 0,
        max: 100,
        value: ecuData.wastegatePosition,
        color: '#00FF88'
      },
      {
        id: 'tps',
        title: 'THROTTLE',
        subtitle: 'TPS (%)',
        unit: '%',
        min: 0,
        max: 100,
        value: ecuData.tpsPosition,
        color: '#FFD700'
      },
      {
        id: 'rpm',
        title: 'ENGINE RPM',
        subtitle: 'Revolutions/min',
        unit: 'RPM',
        min: 0,
        max: 7000,
        value: ecuData.engineRpm,
        color: '#FF6B35',
        warningThreshold: 6000,
        dangerThreshold: 6500
      },
      {
        id: 'target',
        title: 'TARGET BOOST',
        subtitle: 'Calculated (kPa)',
        unit: 'kPa',
        min: 100,
        max: 250,
        value: ecuData.targetBoost,
        color: '#FFD700'
      }
    ];
  }, [ecuData]);

  const getTcuProtectionIndicator = () => {
    if (!ecuData) {
      return {
        icon: Shield,
        text: 'UNKNOWN',
        color: 'text-gray-400',
        borderColor: 'border-gray-400'
      };
    }

    if (ecuData.tcuLimpMode) {
      return {
        icon: Shield,
        text: 'LIMP MODE',
        color: 'text-automotive-danger',
        borderColor: 'border-automotive-danger'
      };
    }

    if (ecuData.tcuProtectionActive) {
      return {
        icon: Shield,
        text: 'PROTECTION',
        color: 'text-automotive-warning',
        borderColor: 'border-automotive-warning'
      };
    }

    return {
      icon: Shield,
      text: 'NORMAL',
      color: 'text-automotive-success',
      borderColor: 'border-automotive-success'
    };
  };

  const tcuIndicator = getTcuProtectionIndicator();
  const IconComponent = tcuIndicator.icon;

  return (
    <div className="min-h-screen bg-automotive-bg">
      {/* Header */}
      <header className="bg-automotive-card border-b border-gray-700 p-4">
        <div className="max-w-7xl mx-auto flex items-center justify-between">
          <div className="flex items-center space-x-4">
            <h1 className="text-2xl font-orbitron font-bold text-white">
              <GaugeIcon className="inline w-7 h-7 text-automotive-accent mr-2" />
              Turbo Control Dashboard
            </h1>
            <div className="flex items-center space-x-2">
              <div className={`connection-dot ${connectionStatus.connected ? 'bg-automotive-success' : 'bg-automotive-danger'}`} />
              <span className="text-sm text-gray-300">{connectionStatus.message}</span>
            </div>
          </div>

          <div className="flex items-center space-x-4">
            <GaugeSettings 
              settings={displaySettings}
              onSettingsChange={setDisplaySettings}
            />
            <Button
              variant="outline"
              className="bg-automotive-card hover:bg-gray-700 text-white border-gray-600"
            >
              <Settings className="w-4 h-4 mr-2" />
              Settings
            </Button>
            <Button
              variant="outline"
              className="bg-automotive-warning hover:bg-orange-600 text-white border-orange-500"
            >
              <Circle className="w-4 h-4 mr-2" />
              Record
            </Button>
          </div>
        </div>
      </header>

      {/* Main Dashboard */}
      <main className="max-w-7xl mx-auto p-6">
        <StatusBanner ecuData={ecuData} connectionStatus={connectionStatus} />

        {/* Gauges Grid */}
        <div 
          className="gap-6 mb-6"
          style={{
            display: 'grid',
            gridTemplateColumns: `repeat(${displaySettings.columns}, 1fr)`
          }}
        >
          {gaugeConfigs
            .filter(config => displaySettings.visibleGauges[config.id as keyof typeof displaySettings.visibleGauges])
            .map((config) => (
              <Gauge 
                key={config.id} 
                config={config} 
                settings={displaySettings}
              />
            ))}

          {/* TCU Protection Status */}
          {displaySettings.visibleGauges.tcuStatus && (
            <div className="gauge-container rounded-xl p-6">
              {displaySettings.showTitles && !displaySettings.compactMode && (
                <div className="text-center mb-4">
                  <h3 className="text-lg font-orbitron font-bold text-white">TCU STATUS</h3>
                  <p className="text-sm text-gray-400">Protection Level</p>
                </div>
              )}
              
              <div className="relative w-48 h-48 mx-auto mb-4 flex items-center justify-center">
                <div className={`w-32 h-32 rounded-full border-4 ${tcuIndicator.borderColor} flex items-center justify-center`}>
                  <div className="text-center">
                    <IconComponent className={`w-10 h-10 ${tcuIndicator.color} mb-2 mx-auto`} />
                    <div className={`text-lg font-orbitron font-bold ${tcuIndicator.color}`}>
                      {tcuIndicator.text}
                    </div>
                  </div>
                </div>
              </div>
              
              {!displaySettings.compactMode && (
                <div className="text-center space-y-1">
                  <div className="text-sm text-gray-400">
                    Torque Req: <span className="text-automotive-accent">
                      {ecuData?.torqueRequest?.toFixed(0) || 0}%
                    </span>
                  </div>
                  <div className="text-xs text-gray-500">Transmission Status</div>
                </div>
              )}
            </div>
          )}
        </div>

        {/* Data Stream and Controls */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <DataStream entries={dataStream} onClear={clearDataStream} />
          <SystemControls 
            connectionStatus={connectionStatus}
            onConnect={connect}
            onDisconnect={disconnect}
          />
          <GaugeDataSimulator 
            onDataUpdate={updateSimulatedData}
            isConnected={connectionStatus.connected}
          />
        </div>
      </main>
    </div>
  );
}
