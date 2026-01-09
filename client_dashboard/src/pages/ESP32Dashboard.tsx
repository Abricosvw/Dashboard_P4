import { useMemo, useState } from 'react';
import { Gauge as GaugeIcon, Shield, Settings, Circle, Wifi, WifiOff } from 'lucide-react';
import { Gauge } from '@/components/Gauge';
import { StatusBanner } from '@/components/StatusBanner';
import { DataStream } from '@/components/DataStream';
import { GaugeSettings, GaugeDisplaySettings } from '@/components/GaugeSettings';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { useESP32WebSocket } from '@/hooks/useESP32WebSocket';
import { GaugeConfig } from '@/types/ecuData';

export default function ESP32Dashboard() {
  const {
    ecuData,
    connectionStatus,
    dataStream,
    clearDataStream,
    connect,
    disconnect,
    sendCommand,
    updateConfig
  } = useESP32WebSocket();

  const [esp32IP, setEsp32IP] = useState('192.168.4.1');
  const [showSettings, setShowSettings] = useState(false);

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

  const handleConnect = () => {
    updateConfig({ esp32_ip: esp32IP });
    connect();
  };

  const handleSendTestCommand = () => {
    sendCommand('test_command');
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
              ESP32 ECU Dashboard
            </h1>
            <div className="flex items-center space-x-2">
              {connectionStatus.connected ? (
                <Wifi className="w-5 h-5 text-automotive-success" />
              ) : (
                <WifiOff className="w-5 h-5 text-automotive-danger" />
              )}
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
              onClick={() => setShowSettings(!showSettings)}
              className="bg-automotive-card hover:bg-gray-700 text-white border-gray-600"
            >
              <Settings className="w-4 h-4 mr-2" />
              ESP32 Settings
            </Button>
            <Button
              variant="outline"
              className="bg-automotive-warning hover:bg-orange-600 text-white border-orange-500"
              onClick={handleSendTestCommand}
              disabled={!connectionStatus.connected}
            >
              <Circle className="w-4 h-4 mr-2" />
              Test Command
            </Button>
          </div>
        </div>
      </header>

      {/* ESP32 Connection Settings */}
      {showSettings && (
        <div className="bg-automotive-card border-b border-gray-700 p-4">
          <div className="max-w-7xl mx-auto">
            <Card className="bg-automotive-card border-gray-600">
              <CardHeader>
                <CardTitle className="text-white">ESP32 Connection Settings</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex items-center space-x-4">
                  <div className="flex-1">
                    <Label htmlFor="esp32-ip" className="text-gray-300">ESP32 IP Address</Label>
                    <Input
                      id="esp32-ip"
                      value={esp32IP}
                      onChange={(e) => setEsp32IP(e.target.value)}
                      placeholder="192.168.4.1"
                      className="bg-automotive-bg border-gray-600 text-white"
                    />
                  </div>
                  <div className="flex space-x-2">
                    <Button
                      onClick={handleConnect}
                      disabled={connectionStatus.connected}
                      className="bg-automotive-success hover:bg-green-600"
                    >
                      Connect
                    </Button>
                    <Button
                      onClick={disconnect}
                      disabled={!connectionStatus.connected}
                      variant="outline"
                      className="border-automotive-danger text-automotive-danger hover:bg-automotive-danger hover:text-white"
                    >
                      Disconnect
                    </Button>
                  </div>
                </div>
                <div className="text-sm text-gray-400">
                  <p>Default ESP32 AP mode IP: 192.168.4.1</p>
                  <p>Make sure to connect to the ESP32 WiFi network first</p>
                </div>
              </CardContent>
            </Card>
          </div>
        </div>
      )}

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

        {/* Data Stream */}
        <div className="grid grid-cols-1 gap-6">
          <DataStream entries={dataStream} onClear={clearDataStream} />
        </div>
      </main>
    </div>
  );
}
