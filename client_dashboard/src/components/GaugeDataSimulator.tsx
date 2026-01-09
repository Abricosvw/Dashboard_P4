import { useState, useEffect } from 'react';
import { Play, Pause, RotateCcw } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Switch } from '@/components/ui/switch';

interface SimulationData {
  mapPressure: number;
  wastegatePosition: number;
  tpsPosition: number;
  engineRpm: number;
  targetBoost: number;
  tcuProtectionActive: boolean;
  tcuLimpMode: boolean;
  torqueRequest: number;
}

interface GaugeDataSimulatorProps {
  onDataUpdate: (data: SimulationData) => void;
  isConnected: boolean;
}

export function GaugeDataSimulator({ onDataUpdate, isConnected }: GaugeDataSimulatorProps) {
  const [isSimulating, setIsSimulating] = useState(false);
  const [simulationSpeed, setSimulationSpeed] = useState(100); // ms
  const [currentData, setCurrentData] = useState<SimulationData>({
    mapPressure: 100,
    wastegatePosition: 50,
    tpsPosition: 0,
    engineRpm: 800,
    targetBoost: 120,
    tcuProtectionActive: false,
    tcuLimpMode: false,
    torqueRequest: 0
  });

  const [scenarios, setScenarios] = useState({
    idle: false,
    cruising: false,
    boost: false,
    protection: false,
    limp: false
  });

  useEffect(() => {
    if (!isSimulating) return;

    const interval = setInterval(() => {
      setCurrentData(prev => {
        let newData = { ...prev };

        // Scenario-based simulation
        if (scenarios.idle) {
          newData.engineRpm = 800 + Math.random() * 100;
          newData.mapPressure = 100 + Math.random() * 5;
          newData.tpsPosition = Math.random() * 10;
          newData.wastegatePosition = 50 + Math.random() * 10;
          newData.targetBoost = 100;
          newData.tcuProtectionActive = false;
          newData.tcuLimpMode = false;
          newData.torqueRequest = 0;
        } else if (scenarios.cruising) {
          newData.engineRpm = 2000 + Math.random() * 500;
          newData.mapPressure = 110 + Math.random() * 20;
          newData.tpsPosition = 20 + Math.random() * 30;
          newData.wastegatePosition = 45 + Math.random() * 20;
          newData.targetBoost = 120 + Math.random() * 10;
          newData.tcuProtectionActive = false;
          newData.tcuLimpMode = false;
          newData.torqueRequest = 30 + Math.random() * 20;
        } else if (scenarios.boost) {
          newData.engineRpm = 3500 + Math.random() * 2000;
          newData.mapPressure = 180 + Math.random() * 50;
          newData.tpsPosition = 70 + Math.random() * 30;
          newData.wastegatePosition = 20 + Math.random() * 30;
          newData.targetBoost = 200 + Math.random() * 30;
          newData.tcuProtectionActive = false;
          newData.tcuLimpMode = false;
          newData.torqueRequest = 70 + Math.random() * 30;
        } else if (scenarios.protection) {
          newData.engineRpm = 4000 + Math.random() * 1500;
          newData.mapPressure = 150 + Math.random() * 30;
          newData.tpsPosition = 50 + Math.random() * 40;
          newData.wastegatePosition = 70 + Math.random() * 20;
          newData.targetBoost = 140 + Math.random() * 20;
          newData.tcuProtectionActive = true;
          newData.tcuLimpMode = false;
          newData.torqueRequest = 40 + Math.random() * 20;
        } else if (scenarios.limp) {
          newData.engineRpm = 1500 + Math.random() * 1000;
          newData.mapPressure = 105 + Math.random() * 15;
          newData.tpsPosition = 10 + Math.random() * 20;
          newData.wastegatePosition = 80 + Math.random() * 15;
          newData.targetBoost = 110 + Math.random() * 10;
          newData.tcuProtectionActive = true;
          newData.tcuLimpMode = true;
          newData.torqueRequest = 10 + Math.random() * 15;
        } else {
          // Random variation mode
          newData.engineRpm = Math.max(500, prev.engineRpm + (Math.random() - 0.5) * 200);
          newData.mapPressure = Math.max(100, Math.min(250, prev.mapPressure + (Math.random() - 0.5) * 10));
          newData.tpsPosition = Math.max(0, Math.min(100, prev.tpsPosition + (Math.random() - 0.5) * 20));
          newData.wastegatePosition = Math.max(0, Math.min(100, prev.wastegatePosition + (Math.random() - 0.5) * 15));
          newData.targetBoost = Math.max(100, Math.min(250, prev.targetBoost + (Math.random() - 0.5) * 5));
        }

        // Ensure realistic bounds
        newData.engineRpm = Math.max(500, Math.min(7000, newData.engineRpm));
        newData.mapPressure = Math.max(100, Math.min(250, newData.mapPressure));
        newData.tpsPosition = Math.max(0, Math.min(100, newData.tpsPosition));
        newData.wastegatePosition = Math.max(0, Math.min(100, newData.wastegatePosition));
        newData.targetBoost = Math.max(100, Math.min(250, newData.targetBoost));
        newData.torqueRequest = Math.max(0, Math.min(100, newData.torqueRequest));

        onDataUpdate(newData);
        return newData;
      });
    }, simulationSpeed);

    return () => clearInterval(interval);
  }, [isSimulating, simulationSpeed, scenarios, onDataUpdate]);

  const handleScenarioChange = (scenario: keyof typeof scenarios) => {
    const newScenarios = Object.keys(scenarios).reduce((acc, key) => {
      acc[key as keyof typeof scenarios] = key === scenario;
      return acc;
    }, {} as typeof scenarios);
    setScenarios(newScenarios);
  };

  const resetData = () => {
    const resetData = {
      mapPressure: 100,
      wastegatePosition: 50,
      tpsPosition: 0,
      engineRpm: 800,
      targetBoost: 120,
      tcuProtectionActive: false,
      tcuLimpMode: false,
      torqueRequest: 0
    };
    setCurrentData(resetData);
    onDataUpdate(resetData);
  };

  return (
    <Card className="border-gray-600 bg-gray-800">
      <CardHeader>
        <CardTitle className="text-white text-lg flex items-center">
          <Play className="w-5 h-5 mr-2 text-automotive-accent" />
          Data Simulator
        </CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        <div className="flex items-center space-x-2">
          <Button
            onClick={() => setIsSimulating(!isSimulating)}
            className={`${
              isSimulating 
                ? 'bg-automotive-warning hover:bg-orange-600' 
                : 'bg-automotive-success hover:bg-green-600'
            } text-white`}
            disabled={isConnected}
          >
            {isSimulating ? (
              <>
                <Pause className="w-4 h-4 mr-2" />
                Stop
              </>
            ) : (
              <>
                <Play className="w-4 h-4 mr-2" />
                Start
              </>
            )}
          </Button>

          <Button
            onClick={resetData}
            variant="outline"
            className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
          >
            <RotateCcw className="w-4 h-4 mr-2" />
            Reset
          </Button>
        </div>

        {isConnected && (
          <div className="p-3 bg-automotive-warning/20 border border-automotive-warning rounded">
            <p className="text-sm text-automotive-warning">
              Simulator disabled - ECU connection active
            </p>
          </div>
        )}

        <div>
          <Label className="text-gray-300">Update Rate (ms)</Label>
          <Input
            type="number"
            value={simulationSpeed}
            onChange={(e) => setSimulationSpeed(Number(e.target.value))}
            min="50"
            max="1000"
            step="50"
            className="mt-2 bg-gray-700 border-gray-600 text-white"
            disabled={isConnected}
          />
        </div>

        <div>
          <Label className="text-gray-300 mb-3 block">Scenarios</Label>
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Idle</Label>
              <Switch
                checked={scenarios.idle}
                onCheckedChange={() => handleScenarioChange('idle')}
                disabled={isConnected || !isSimulating}
              />
            </div>
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Cruising</Label>
              <Switch
                checked={scenarios.cruising}
                onCheckedChange={() => handleScenarioChange('cruising')}
                disabled={isConnected || !isSimulating}
              />
            </div>
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Boost Active</Label>
              <Switch
                checked={scenarios.boost}
                onCheckedChange={() => handleScenarioChange('boost')}
                disabled={isConnected || !isSimulating}
              />
            </div>
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">TCU Protection</Label>
              <Switch
                checked={scenarios.protection}
                onCheckedChange={() => handleScenarioChange('protection')}
                disabled={isConnected || !isSimulating}
              />
            </div>
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Limp Mode</Label>
              <Switch
                checked={scenarios.limp}
                onCheckedChange={() => handleScenarioChange('limp')}
                disabled={isConnected || !isSimulating}
              />
            </div>
          </div>
        </div>

        <div className="text-xs text-gray-500 pt-2 border-t border-gray-600">
          Current: RPM {currentData.engineRpm.toFixed(0)} | MAP {currentData.mapPressure.toFixed(1)}kPa | TPS {currentData.tpsPosition.toFixed(1)}%
        </div>
      </CardContent>
    </Card>
  );
}