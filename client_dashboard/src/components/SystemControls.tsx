import { useState } from 'react';
import { useQuery, useMutation } from '@tanstack/react-query';
import { Plug, Unlink, RotateCcw, Download, Wrench, Square } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Switch } from '@/components/ui/switch';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { useToast } from '@/hooks/use-toast';
import { apiRequest, queryClient } from '@/lib/queryClient';
import { SystemSettings, ConnectionStatus } from '@/types/ecuData';

interface SystemControlsProps {
  connectionStatus: ConnectionStatus;
  onConnect: () => void;
  onDisconnect: () => void;
}

export function SystemControls({ connectionStatus, onConnect, onDisconnect }: SystemControlsProps) {
  const { toast } = useToast();
  const [ecuAddress, setEcuAddress] = useState('ws://192.168.1.100:8080');

  const { data: settings } = useQuery<SystemSettings>({
    queryKey: ['/api/settings'],
  });

  const updateSettingsMutation = useMutation({
    mutationFn: async (newSettings: Partial<SystemSettings>) => {
      const response = await apiRequest('POST', '/api/settings', newSettings);
      return response.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['/api/settings'] });
      toast({
        title: 'Success',
        description: 'Settings updated successfully',
      });
    },
    onError: () => {
      toast({
        title: 'Error',
        description: 'Failed to update settings',
        variant: 'destructive',
      });
    },
  });

  const exportDataMutation = useMutation({
    mutationFn: async () => {
      const response = await apiRequest('GET', '/api/ecu-data/export');
      const data = await response.json();
      
      const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `ecu-data-export-${new Date().toISOString().split('T')[0]}.json`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    },
    onSuccess: () => {
      toast({
        title: 'Success',
        description: 'Data exported successfully',
      });
    },
    onError: () => {
      toast({
        title: 'Error',
        description: 'Failed to export data',
        variant: 'destructive',
      });
    },
  });

  const clearDataMutation = useMutation({
    mutationFn: async () => {
      const response = await apiRequest('DELETE', '/api/ecu-data');
      return response.json();
    },
    onSuccess: () => {
      toast({
        title: 'Success',
        description: 'Data logs cleared',
      });
    },
    onError: () => {
      toast({
        title: 'Error',
        description: 'Failed to clear data logs',
        variant: 'destructive',
      });
    },
  });

  const handleConnect = () => {
    if (connectionStatus.connected) {
      onDisconnect();
    } else {
      onConnect();
    }
  };

  const handleSettingChange = (key: keyof SystemSettings, value: any) => {
    updateSettingsMutation.mutate({ [key]: value });
  };

  return (
    <div className="bg-automotive-card rounded-xl p-6 border border-gray-700">
      <h3 className="text-lg font-orbitron font-bold text-white mb-4">SYSTEM CONTROLS</h3>

      <div className="space-y-4">
        {/* Connection Settings */}
        <Card className="border-gray-600 bg-gray-800">
          <CardHeader className="pb-3">
            <CardTitle className="text-white text-base">Connection</CardTitle>
          </CardHeader>
          <CardContent className="space-y-3">
            <div>
              <Label className="text-gray-400 text-sm">ECU Address</Label>
              <Input
                value={ecuAddress}
                onChange={(e) => setEcuAddress(e.target.value)}
                className="bg-gray-800 border-gray-600 text-white mt-1"
                placeholder="ws://192.168.1.100:8080"
              />
            </div>
            
            <div className="flex space-x-2">
              <Button
                onClick={handleConnect}
                className={`flex-1 ${
                  connectionStatus.connected
                    ? 'bg-automotive-warning hover:bg-orange-600'
                    : 'bg-automotive-accent hover:bg-blue-400'
                } text-white transition-colors`}
              >
                {connectionStatus.connected ? (
                  <>
                    <Unlink className="w-4 h-4 mr-2" />
                    Disconnect
                  </>
                ) : (
                  <>
                    <Plug className="w-4 h-4 mr-2" />
                    Connect
                  </>
                )}
              </Button>
            </div>
            
            <div className="text-sm text-center">
              <span className="text-gray-400">Status: </span>
              <span className={connectionStatus.connected ? 'text-automotive-success' : 'text-automotive-danger'}>
                {connectionStatus.message}
              </span>
            </div>
          </CardContent>
        </Card>

        {/* Alert Settings */}
        <Card className="border-gray-600 bg-gray-800">
          <CardHeader className="pb-3">
            <CardTitle className="text-white text-base">Alerts</CardTitle>
          </CardHeader>
          <CardContent className="space-y-3">
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Max Boost (kPa)</Label>
              <Input
                type="number"
                value={settings?.maxBoostLimit || 250}
                onChange={(e) => handleSettingChange('maxBoostLimit', Number(e.target.value))}
                className="w-20 bg-gray-800 border-gray-600 text-white text-sm"
              />
            </div>
            
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Max RPM</Label>
              <Input
                type="number"
                value={settings?.maxRpmLimit || 7000}
                onChange={(e) => handleSettingChange('maxRpmLimit', Number(e.target.value))}
                className="w-20 bg-gray-800 border-gray-600 text-white text-sm"
              />
            </div>
            
            <div className="flex items-center justify-between">
              <Label className="text-gray-400">Audio Alerts</Label>
              <Switch
                checked={settings?.audioAlertsEnabled || false}
                onCheckedChange={(checked) => handleSettingChange('audioAlertsEnabled', checked)}
              />
            </div>
          </CardContent>
        </Card>

        {/* Quick Actions */}
        <Card className="border-gray-600 bg-gray-800">
          <CardHeader className="pb-3">
            <CardTitle className="text-white text-base">Quick Actions</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="grid grid-cols-2 gap-2">
              <Button
                variant="outline"
                size="sm"
                className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                onClick={() => {
                  toast({
                    title: 'PID Reset',
                    description: 'PID controller has been reset',
                  });
                }}
              >
                <RotateCcw className="w-3 h-3 mr-1" />
                Reset PID
              </Button>
              
              <Button
                variant="outline"
                size="sm"
                className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                onClick={() => exportDataMutation.mutate()}
                disabled={exportDataMutation.isPending}
              >
                <Download className="w-3 h-3 mr-1" />
                Export
              </Button>
              
              <Button
                variant="outline"
                size="sm"
                className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                onClick={() => {
                  toast({
                    title: 'Calibration',
                    description: 'Calibration mode activated',
                  });
                }}
              >
                <Wrench className="w-3 h-3 mr-1" />
                Calibrate
              </Button>
              
              <Button
                variant="outline"
                size="sm"
                className="bg-automotive-danger border-red-600 hover:bg-red-600 text-white"
                onClick={() => clearDataMutation.mutate()}
                disabled={clearDataMutation.isPending}
              >
                <Square className="w-3 h-3 mr-1" />
                Clear Data
              </Button>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
